/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/Passes/WDGlobalGIScreenProbeTraceRTPass.h>
#include <Render/WDGlobalGIFeatureProcessor.h>

#include <AzCore/Math/MathUtils.h>
#include <Atom/RHI/DispatchRaysItem.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<WDGlobalGIScreenProbeTraceRTPass> WDGlobalGIScreenProbeTraceRTPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew WDGlobalGIScreenProbeTraceRTPass(descriptor);
        }

        WDGlobalGIScreenProbeTraceRTPass::WDGlobalGIScreenProbeTraceRTPass(const RPI::PassDescriptor& descriptor)
            : RayTracingPass(descriptor)
        {
        }

        WDGlobalGIFeatureProcessor* WDGlobalGIScreenProbeTraceRTPass::GetFeatureProcessor() const
        {
            RPI::Scene* scene = m_pipeline ? m_pipeline->GetScene() : nullptr;
            return scene ? scene->GetFeatureProcessor<WDGlobalGIFeatureProcessor>() : nullptr;
        }

        bool WDGlobalGIScreenProbeTraceRTPass::IsEnabled() const
        {
            // Covers the "no ray tracing support on this device" case too - the base constructor calls
            // SetEnabled(false) when RHISystemInterface::GetRayTracingSupport() has no devices, and
            // RenderPass::IsEnabled() (called by RayTracingPass::IsEnabled()) honours that.
            if (!RayTracingPass::IsEnabled())
            {
                return false;
            }
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            return fp && fp->GetUseScreenProbes() && fp->GetUseHardwareRT();
        }

        void WDGlobalGIScreenProbeTraceRTPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            // Imports the TLAS and declares the connected "Depth" slot attachment.
            RayTracingPass::SetupFrameGraphDependencies(frameGraph);

            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            if (!fp)
            {
                return;
            }

            auto importUse = [&](const Data::Instance<RPI::AttachmentImage>& image,
                                 const RHI::ImageViewDescriptor& viewDesc, RHI::ScopeAttachmentAccess access)
            {
                if (!image)
                {
                    return;
                }
                const RHI::AttachmentId id = image->GetAttachmentId();
                if (!frameGraph.GetAttachmentDatabase().IsAttachmentValid(id))
                {
                    frameGraph.GetAttachmentDatabase().ImportImage(id, image->GetRHIImage());
                }
                RHI::ImageScopeAttachmentDescriptor desc;
                desc.m_attachmentId = id;
                desc.m_imageViewDescriptor = viewDesc;
                desc.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;
                frameGraph.UseShaderAttachment(desc, access, RHI::ScopeAttachmentStage::RayTracingShader);
            };

            // Same ping-pong scheme as the compute pass (WDGlobalGIScreenProbeTracePass): write "current",
            // read reprojected "previous". Mutual exclusion (IsEnabled() above) means only one of the two
            // passes ever touches these attachments in a given frame.
            const uint32_t frameIndex = fp->GetClipmap().GetShaderConstants().m_frameIndex;
            importUse(fp->GetClipmap().GetScreenProbeAtlasCurrent(frameIndex), WDGlobalGIClipmap::GetScreenProbeAtlasViewDescriptor(), RHI::ScopeAttachmentAccess::ReadWrite);
            importUse(fp->GetClipmap().GetScreenProbeAtlasPrev(frameIndex), WDGlobalGIClipmap::GetScreenProbeAtlasViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            importUse(fp->GetClipmap().GetScreenProbeDepthAtlas(), WDGlobalGIClipmap::GetScreenProbeAtlasViewDescriptor(), RHI::ScopeAttachmentAccess::ReadWrite);
        }

        void WDGlobalGIScreenProbeTraceRTPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            // Builds the shader table, the RayTracingGlobal/Scene/View/Scene SRGs, and a default 1x1x1
            // dispatch (overridden below with the real atlas-sized dispatch).
            RayTracingPass::CompileResources(context);

            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            if (!fp || !m_rayGenerationShader)
            {
                return;
            }

            if (!m_passSrg)
            {
                m_srgLayout = m_rayGenerationShader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Pass);
                if (!m_srgLayout)
                {
                    AZ_Error("WDGlobalGIScreenProbeTraceRTPass", false, "Shader has no SRG_PerPass layout");
                    return;
                }
                m_passSrg = RPI::ShaderResourceGroup::Create(
                    m_rayGenerationShader->GetAsset(), m_rayGenerationShader->GetSupervariantIndex(), m_srgLayout->GetName());
            }
            if (!m_passSrg)
            {
                return;
            }

            // Shared addressing / lighting / tuning constants - the same helper the compute GI passes use.
            fp->GetClipmap().FillSharedConstants(m_passSrg.get());

            const RHI::ShaderResourceGroupLayout* layout = m_passSrg->GetLayout();
            auto setImage = [&](const char* name, const RHI::ImageView* view)
            {
                if (!view)
                {
                    return;
                }
                RHI::ShaderInputImageIndex index = layout->FindShaderInputImageIndex(AZ::Name(name));
                if (index.IsValid())
                {
                    m_passSrg->SetImageView(index, view);
                }
            };

            RPI::PassAttachmentBinding* depthBinding = FindAttachmentBinding(AZ::Name("Depth"));
            if (depthBinding && depthBinding->GetAttachment())
            {
                setImage("m_depth", context.GetImageView(depthBinding->GetAttachment()->GetAttachmentId()));
            }

            const uint32_t frameIndex = fp->GetClipmap().GetShaderConstants().m_frameIndex;
            const auto& atlasCurrent = fp->GetClipmap().GetScreenProbeAtlasCurrent(frameIndex);
            const auto& atlasPrev = fp->GetClipmap().GetScreenProbeAtlasPrev(frameIndex);
            const auto& depthAtlas = fp->GetClipmap().GetScreenProbeDepthAtlas();
            setImage("m_screenProbeAtlas", atlasCurrent ? atlasCurrent->GetImageView() : nullptr);
            setImage("m_screenProbeHistory", atlasPrev ? atlasPrev->GetImageView() : nullptr);
            setImage("m_screenProbeDepthAtlas", depthAtlas ? depthAtlas->GetImageView() : nullptr);

            // Screen resolution -> probe tile counts (clamped to the atlas max), same formula as
            // WDGlobalGIScreenProbeTracePass::BindGIResources().
            uint32_t screenW = 1, screenH = 1;
            if (depthBinding && depthBinding->GetAttachment())
            {
                const RHI::ImageDescriptor& imageDesc = depthBinding->GetAttachment()->m_descriptor.m_image;
                screenW = imageDesc.m_size.m_width;
                screenH = imageDesc.m_size.m_height;
            }

            const uint32_t tile = WDGlobalGILimits::ScreenProbeTileSize;
            const uint32_t octa = WDGlobalGILimits::ScreenProbeOctaRes;
            const uint32_t tilesX = AZ::GetMin((screenW + tile - 1) / tile, WDGlobalGILimits::ScreenProbeAtlasMaxWidth / octa);
            const uint32_t tilesY = AZ::GetMin((screenH + tile - 1) / tile, WDGlobalGILimits::ScreenProbeAtlasMaxHeight / octa);

            const uint32_t screenProbeInfo[4] = { screenW, screenH, tilesX, tilesY };
            if (RHI::ShaderInputConstantIndex infoIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_screenProbeInfo")); infoIndex.IsValid())
            {
                m_passSrg->SetConstantRaw(infoIndex, screenProbeInfo, sizeof(screenProbeInfo));
            }
            const float temporal = fp->GetScreenProbeTemporal();
            if (RHI::ShaderInputConstantIndex tIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_screenProbeTemporal")); tIndex.IsValid())
            {
                m_passSrg->SetConstantRaw(tIndex, &temporal, sizeof(temporal));
            }

            // Same denoiser gap-closes as the SDF trace pass (variance-adaptive blend + luminance clamp).
            const WDGlobalGIConfiguration& config = fp->GetConfiguration();
            if (RHI::ShaderInputConstantIndex idx = layout->FindShaderInputConstantIndex(AZ::Name("m_screenProbeVarianceScale")); idx.IsValid())
            {
                m_passSrg->SetConstantRaw(idx, &config.m_screenProbeVarianceScale, sizeof(config.m_screenProbeVarianceScale));
            }
            if (RHI::ShaderInputConstantIndex idx = layout->FindShaderInputConstantIndex(AZ::Name("m_screenProbeLuminanceClamp")); idx.IsValid())
            {
                m_passSrg->SetConstant(idx, config.m_screenProbeLuminanceClamp);
            }
            if (RHI::ShaderInputConstantIndex idx = layout->FindShaderInputConstantIndex(AZ::Name("m_screenProbeMaxLuminance")); idx.IsValid())
            {
                m_passSrg->SetConstantRaw(idx, &config.m_screenProbeMaxLuminance, sizeof(config.m_screenProbeMaxLuminance));
            }

            if (!m_passSrg->IsQueuedForCompile())
            {
                m_passSrg->Compile();
            }

            m_rayTracingSRGsToBind.push_back(m_passSrg->GetRHIShaderResourceGroup());

            // Dispatch exactly the live probe-atlas region (tilesX*octa x tilesY*octa), not the fixed max
            // atlas allocation - same thread count the compute pass computes in GetDispatchThreadCount().
            RHI::DispatchRaysDirect dispatchRaysArgs{
                AZ::GetMax(tilesX * octa, 1u), AZ::GetMax(tilesY * octa, 1u), 1u
            };
            m_dispatchRaysItem.SetArguments(dispatchRaysArgs);
        }
    } // namespace Render
} // namespace AZ
