/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/Passes/WDGlobalGIClearPass.h>
#include <Render/WDGlobalGIFeatureProcessor.h>

#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<WDGlobalGIClearPass> WDGlobalGIClearPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew WDGlobalGIClearPass(descriptor);
        }

        WDGlobalGIClearPass::WDGlobalGIClearPass(const RPI::PassDescriptor& descriptor)
            : WDGlobalGIComputePass(descriptor, "Shaders/WDGlobalGI/WDGlobalGIClear.azshader")
        {
        }

        void WDGlobalGIClearPass::SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph)
        {
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            if (!fp)
            {
                return;
            }

            auto importUse = [&](const Data::Instance<RPI::AttachmentImage>& image,
                                 const RHI::ImageViewDescriptor& viewDesc)
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
                frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::ComputeShader);
            };

            const auto& clipmap = fp->GetClipmap();
            importUse(clipmap.GetRadianceClipmap(), WDGlobalGIClipmap::GetRadianceViewDescriptor());
            importUse(clipmap.GetIrradianceClipmap(), WDGlobalGIClipmap::GetIrradianceViewDescriptor());
            importUse(clipmap.GetSdfClipmap(), WDGlobalGIClipmap::GetSdfViewDescriptor());
            importUse(clipmap.GetVoxelNormalClipmap(), WDGlobalGIClipmap::GetVoxelNormalViewDescriptor());
            importUse(clipmap.GetAlbedoClipmap(), WDGlobalGIClipmap::GetAlbedoViewDescriptor());
            importUse(clipmap.GetAnisoRadianceClipmap(), WDGlobalGIClipmap::GetAnisoRadianceViewDescriptor());
        }

        void WDGlobalGIClearPass::BindGIResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg)
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

            const auto& clipmap = fp->GetClipmap();
            setImage("m_radianceClipmap", clipmap.GetRadianceClipmap());
            setImage("m_irradianceClipmap", clipmap.GetIrradianceClipmap());
            setImage("m_sdfClipmap", clipmap.GetSdfClipmap());
            setImage("m_voxelNormalClipmap", clipmap.GetVoxelNormalClipmap());
            setImage("m_albedoClipmap", clipmap.GetAlbedoClipmap());
            setImage("m_anisoRadianceClipmap", clipmap.GetAnisoRadianceClipmap());
        }

        void WDGlobalGIClearPass::GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const
        {
            uint32_t cascadeCount = 1;
            if (WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor())
            {
                cascadeCount = fp->GetClipmap().GetShaderConstants().m_cascadeCount;
            }
            x = WDGlobalGILimits::GridResolutionX;
            y = WDGlobalGILimits::GridResolutionY;
            z = WDGlobalGILimits::GridResolutionZ * cascadeCount;
        }
    } // namespace Render
} // namespace AZ
