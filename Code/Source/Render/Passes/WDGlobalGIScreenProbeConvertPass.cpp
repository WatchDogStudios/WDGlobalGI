/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/Passes/WDGlobalGIScreenProbeConvertPass.h>
#include <Render/WDGlobalGIFeatureProcessor.h>

#include <AzCore/Math/MathUtils.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<WDGlobalGIScreenProbeConvertPass> WDGlobalGIScreenProbeConvertPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew WDGlobalGIScreenProbeConvertPass(descriptor);
        }

        WDGlobalGIScreenProbeConvertPass::WDGlobalGIScreenProbeConvertPass(const RPI::PassDescriptor& descriptor)
            : WDGlobalGIComputePass(descriptor, "Shaders/WDGlobalGI/WDGlobalGIScreenProbeConvert.azshader")
        {
        }

        bool WDGlobalGIScreenProbeConvertPass::IsEnabled() const
        {
            if (!WDGlobalGIComputePass::IsEnabled())
            {
                return false;
            }
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            return fp && fp->GetUseScreenProbes();
        }

        void WDGlobalGIScreenProbeConvertPass::SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph)
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
            importUse(fp->GetClipmap().GetScreenProbeAtlasCurrent(frameIndex), WDGlobalGIClipmap::GetScreenProbeAtlasViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            importUse(fp->GetClipmap().GetScreenProbeSH(), WDGlobalGIClipmap::GetScreenProbeShViewDescriptor(), RHI::ScopeAttachmentAccess::ReadWrite);
        }

        void WDGlobalGIScreenProbeConvertPass::BindGIResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg)
        {
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            if (!fp)
            {
                return;
            }

            const RHI::ShaderResourceGroupLayout* layout = srg->GetLayout();
            auto setImage = [&](const char* name, const Data::Instance<RPI::AttachmentImage>& image)
            {
                if (!image)
                {
                    return;
                }
                RHI::ShaderInputImageIndex index = layout->FindShaderInputImageIndex(AZ::Name(name));
                if (index.IsValid())
                {
                    srg->SetImageView(index, image->GetImageView());
                }
            };

            const uint32_t frameIndex = fp->GetClipmap().GetShaderConstants().m_frameIndex;
            setImage("m_screenProbeAtlas", fp->GetClipmap().GetScreenProbeAtlasCurrent(frameIndex));
            setImage("m_screenProbeSH", fp->GetClipmap().GetScreenProbeSH());

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
        }

        void WDGlobalGIScreenProbeConvertPass::GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const
        {
            x = AZ::GetMax(m_tilesX, 1u);
            y = AZ::GetMax(m_tilesY, 1u);
            z = 1;
        }
    } // namespace Render
} // namespace AZ
