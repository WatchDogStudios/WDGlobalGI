/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/Passes/WDGlobalGIScreenProbeIntegratePass.h>
#include <Render/WDGlobalGIFeatureProcessor.h>

#include <AzCore/Math/MathUtils.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<WDGlobalGIScreenProbeIntegratePass> WDGlobalGIScreenProbeIntegratePass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew WDGlobalGIScreenProbeIntegratePass(descriptor);
        }

        WDGlobalGIScreenProbeIntegratePass::WDGlobalGIScreenProbeIntegratePass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        WDGlobalGIFeatureProcessor* WDGlobalGIScreenProbeIntegratePass::GetFeatureProcessor() const
        {
            RPI::Scene* scene = m_pipeline ? m_pipeline->GetScene() : nullptr;
            return scene ? scene->GetFeatureProcessor<WDGlobalGIFeatureProcessor>() : nullptr;
        }

        bool WDGlobalGIScreenProbeIntegratePass::IsEnabled() const
        {
            if (!FullscreenTrianglePass::IsEnabled())
            {
                return false;
            }
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            return fp && fp->ShouldRender() && fp->GetUseScreenProbes();
        }

        void WDGlobalGIScreenProbeIntegratePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            FullscreenTrianglePass::SetupFrameGraphDependencies(frameGraph);

            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            if (!fp)
            {
                return;
            }

            auto importRead = [&](const Data::Instance<RPI::AttachmentImage>& image, const RHI::ImageViewDescriptor& viewDesc)
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
                frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::FragmentShader);
            };

            const uint32_t frameIndex = fp->GetClipmap().GetShaderConstants().m_frameIndex;
            importRead(fp->GetClipmap().GetScreenProbeSHBlurred(), WDGlobalGIClipmap::GetScreenProbeShViewDescriptor());
            importRead(fp->GetClipmap().GetScreenProbeAtlasCurrent(frameIndex), WDGlobalGIClipmap::GetScreenProbeAtlasViewDescriptor());
            importRead(fp->GetClipmap().GetScreenProbeDepthAtlas(), WDGlobalGIClipmap::GetScreenProbeAtlasViewDescriptor());
        }

        void WDGlobalGIScreenProbeIntegratePass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            Data::Instance<RPI::ShaderResourceGroup> srg = GetShaderResourceGroup();

            if (fp && srg)
            {
                fp->GetClipmap().FillSharedConstants(srg.get());

                const RHI::ShaderResourceGroupLayout* layout = srg->GetLayout();

                auto setImage = [&](const char* name, const Data::Instance<RPI::AttachmentImage>& image)
                {
                    if (!image)
                    {
                        return;
                    }
                    if (RHI::ShaderInputImageIndex index = layout->FindShaderInputImageIndex(AZ::Name(name)); index.IsValid())
                    {
                        srg->SetImageView(index, image->GetImageView());
                    }
                };
                const uint32_t frameIndex = fp->GetClipmap().GetShaderConstants().m_frameIndex;
                setImage("m_screenProbeSH", fp->GetClipmap().GetScreenProbeSHBlurred()); // denoised SH
                setImage("m_screenProbeAtlas", fp->GetClipmap().GetScreenProbeAtlasCurrent(frameIndex));
                setImage("m_screenProbeDepthAtlas", fp->GetClipmap().GetScreenProbeDepthAtlas());

                const float specular = fp->GetScreenProbeSpecular();
                if (RHI::ShaderInputConstantIndex sIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_screenProbeSpecular")); sIndex.IsValid())
                {
                    srg->SetConstantRaw(sIndex, &specular, sizeof(specular));
                }

                const float specularRoughness = fp->GetConfiguration().m_specularRoughness;
                if (RHI::ShaderInputConstantIndex rIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_specularRoughness")); rIndex.IsValid())
                {
                    srg->SetConstantRaw(rIndex, &specularRoughness, sizeof(specularRoughness));
                }

                // Screen resolution -> probe tile counts (must match the trace pass).
                uint32_t screenW = 1, screenH = 1;
                if (RPI::PassAttachmentBinding* binding = FindAttachmentBinding(AZ::Name("DepthInput")); binding && binding->GetAttachment())
                {
                    const RHI::ImageDescriptor& imageDesc = binding->GetAttachment()->m_descriptor.m_image;
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
                    srg->SetConstantRaw(infoIndex, screenProbeInfo, sizeof(screenProbeInfo));
                }
            }

            FullscreenTrianglePass::CompileResources(context);
        }
    } // namespace Render
} // namespace AZ
