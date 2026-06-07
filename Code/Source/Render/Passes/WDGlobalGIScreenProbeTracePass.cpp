/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/Passes/WDGlobalGIScreenProbeTracePass.h>
#include <Render/WDGlobalGIFeatureProcessor.h>

#include <AzCore/Math/MathUtils.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<WDGlobalGIScreenProbeTracePass> WDGlobalGIScreenProbeTracePass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew WDGlobalGIScreenProbeTracePass(descriptor);
        }

        WDGlobalGIScreenProbeTracePass::WDGlobalGIScreenProbeTracePass(const RPI::PassDescriptor& descriptor)
            : WDGlobalGIComputePass(descriptor, "Shaders/WDGlobalGI/WDGlobalGIScreenProbeTrace.azshader")
        {
        }

        bool WDGlobalGIScreenProbeTracePass::IsEnabled() const
        {
            if (!WDGlobalGIComputePass::IsEnabled())
            {
                return false;
            }
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            return fp && fp->GetUseScreenProbes();
        }

        void WDGlobalGIScreenProbeTracePass::SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph)
        {
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
                frameGraph.UseShaderAttachment(desc, access, RHI::ScopeAttachmentStage::ComputeShader);
            };

            const uint32_t frameIndex = fp->GetClipmap().GetShaderConstants().m_frameIndex;
            importUse(fp->GetClipmap().GetRadianceClipmap(), WDGlobalGIClipmap::GetRadianceViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            importUse(fp->GetClipmap().GetSdfClipmap(), WDGlobalGIClipmap::GetSdfViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            importUse(fp->GetClipmap().GetVoxelNormalClipmap(), WDGlobalGIClipmap::GetVoxelNormalViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            importUse(fp->GetClipmap().GetAnisoRadianceClipmap(), WDGlobalGIClipmap::GetAnisoRadianceViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            importUse(fp->GetClipmap().GetIrradianceClipmap(), WDGlobalGIClipmap::GetIrradianceViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            // Ping-pong: write the "current" atlas, read the "previous" one (reprojected) for history.
            importUse(fp->GetClipmap().GetScreenProbeAtlasCurrent(frameIndex), WDGlobalGIClipmap::GetScreenProbeAtlasViewDescriptor(), RHI::ScopeAttachmentAccess::ReadWrite);
            importUse(fp->GetClipmap().GetScreenProbeAtlasPrev(frameIndex), WDGlobalGIClipmap::GetScreenProbeAtlasViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            // Depth/visibility moments atlas (Chebyshev), written fresh each frame.
            importUse(fp->GetClipmap().GetScreenProbeDepthAtlas(), WDGlobalGIClipmap::GetScreenProbeAtlasViewDescriptor(), RHI::ScopeAttachmentAccess::ReadWrite);
        }

        void WDGlobalGIScreenProbeTracePass::BindGIResources(const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg)
        {
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            if (!fp)
            {
                return;
            }

            const RHI::ShaderResourceGroupLayout* layout = srg->GetLayout();
            auto setImage = [&](const char* name, const RHI::ImageView* view)
            {
                if (!view)
                {
                    return;
                }
                RHI::ShaderInputImageIndex index = layout->FindShaderInputImageIndex(AZ::Name(name));
                if (index.IsValid())
                {
                    srg->SetImageView(index, view);
                }
            };

            const uint32_t frameIndex = fp->GetClipmap().GetShaderConstants().m_frameIndex;
            setImage("m_depth", GetInputImageView(context, AZ::Name("Depth")));
            const auto& radiance = fp->GetClipmap().GetRadianceClipmap();
            const auto& sdf = fp->GetClipmap().GetSdfClipmap();
            const auto& voxelNormal = fp->GetClipmap().GetVoxelNormalClipmap();
            const auto& anisoRadiance = fp->GetClipmap().GetAnisoRadianceClipmap();
            const auto& irradiance = fp->GetClipmap().GetIrradianceClipmap();
            const auto& atlasCurrent = fp->GetClipmap().GetScreenProbeAtlasCurrent(frameIndex);
            const auto& atlasPrev = fp->GetClipmap().GetScreenProbeAtlasPrev(frameIndex);
            setImage("m_radianceClipmap", radiance ? radiance->GetImageView() : nullptr);
            setImage("m_sdfClipmap", sdf ? sdf->GetImageView() : nullptr);
            setImage("m_voxelNormalClipmap", voxelNormal ? voxelNormal->GetImageView() : nullptr);
            setImage("m_anisoRadianceClipmap", anisoRadiance ? anisoRadiance->GetImageView() : nullptr);
            setImage("m_irradianceClipmap", irradiance ? irradiance->GetImageView() : nullptr);
            setImage("m_screenProbeAtlas", atlasCurrent ? atlasCurrent->GetImageView() : nullptr);
            setImage("m_screenProbeHistory", atlasPrev ? atlasPrev->GetImageView() : nullptr);
            const auto& depthAtlas = fp->GetClipmap().GetScreenProbeDepthAtlas();
            setImage("m_screenProbeDepthAtlas", depthAtlas ? depthAtlas->GetImageView() : nullptr);

            // Screen resolution -> probe tile counts (clamped to the atlas max).
            uint32_t screenW = 1, screenH = 1;
            if (RPI::PassAttachmentBinding* binding = FindAttachmentBinding(AZ::Name("Depth")); binding && binding->GetAttachment())
            {
                const RHI::ImageDescriptor& imageDesc = binding->GetAttachment()->m_descriptor.m_image;
                screenW = imageDesc.m_size.m_width;
                screenH = imageDesc.m_size.m_height;
            }

            const uint32_t tile = WDGlobalGILimits::ScreenProbeTileSize;
            const uint32_t octa = WDGlobalGILimits::ScreenProbeOctaRes;
            m_tilesX = AZ::GetMin((screenW + tile - 1) / tile, WDGlobalGILimits::ScreenProbeAtlasMaxWidth / octa);
            m_tilesY = AZ::GetMin((screenH + tile - 1) / tile, WDGlobalGILimits::ScreenProbeAtlasMaxHeight / octa);

            const uint32_t screenProbeInfo[4] = { screenW, screenH, m_tilesX, m_tilesY };
            if (RHI::ShaderInputConstantIndex infoIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_screenProbeInfo")); infoIndex.IsValid())
            {
                srg->SetConstantRaw(infoIndex, screenProbeInfo, sizeof(screenProbeInfo));
            }
            const float temporal = fp->GetScreenProbeTemporal();
            if (RHI::ShaderInputConstantIndex tIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_screenProbeTemporal")); tIndex.IsValid())
            {
                srg->SetConstantRaw(tIndex, &temporal, sizeof(temporal));
            }
        }

        void WDGlobalGIScreenProbeTracePass::GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const
        {
            x = AZ::GetMax(m_tilesX * WDGlobalGILimits::ScreenProbeOctaRes, 1u);
            y = AZ::GetMax(m_tilesY * WDGlobalGILimits::ScreenProbeOctaRes, 1u);
            z = 1;
        }
    } // namespace Render
} // namespace AZ
