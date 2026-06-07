/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/Passes/WDGlobalGISdfUpdatePass.h>
#include <Render/WDGlobalGIFeatureProcessor.h>

#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<WDGlobalGISdfUpdatePass> WDGlobalGISdfUpdatePass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew WDGlobalGISdfUpdatePass(descriptor);
        }

        WDGlobalGISdfUpdatePass::WDGlobalGISdfUpdatePass(const RPI::PassDescriptor& descriptor)
            : WDGlobalGIComputePass(descriptor, "Shaders/WDGlobalGI/WDGlobalGISdfUpdate.azshader")
        {
        }

        void WDGlobalGISdfUpdatePass::SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph)
        {
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            if (!fp)
            {
                return;
            }

            const auto& radiance = fp->GetClipmap().GetRadianceClipmap();
            const auto& sdf = fp->GetClipmap().GetSdfClipmap();
            if (!radiance || !sdf)
            {
                return;
            }

            // Read the voxel occupancy...
            {
                const RHI::AttachmentId id = radiance->GetAttachmentId();
                if (!frameGraph.GetAttachmentDatabase().IsAttachmentValid(id))
                {
                    frameGraph.GetAttachmentDatabase().ImportImage(id, radiance->GetRHIImage());
                }
                RHI::ImageScopeAttachmentDescriptor desc;
                desc.m_attachmentId = id;
                desc.m_imageViewDescriptor = WDGlobalGIClipmap::GetRadianceViewDescriptor();
                desc.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;
                frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::ComputeShader);
            }

            // ...update the distance field in place.
            {
                const RHI::AttachmentId id = sdf->GetAttachmentId();
                if (!frameGraph.GetAttachmentDatabase().IsAttachmentValid(id))
                {
                    frameGraph.GetAttachmentDatabase().ImportImage(id, sdf->GetRHIImage());
                }
                RHI::ImageScopeAttachmentDescriptor desc;
                desc.m_attachmentId = id;
                desc.m_imageViewDescriptor = WDGlobalGIClipmap::GetSdfViewDescriptor();
                desc.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;
                frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::ComputeShader);
            }
        }

        void WDGlobalGISdfUpdatePass::BindGIResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg)
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

            setImage("m_radianceClipmap", fp->GetClipmap().GetRadianceClipmap());
            setImage("m_sdfClipmap", fp->GetClipmap().GetSdfClipmap());
        }

        void WDGlobalGISdfUpdatePass::GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const
        {
            uint32_t cascadeCount = 1;
            if (WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor())
            {
                cascadeCount = fp->GetClipmap().GetShaderConstants().m_cascadeCount;
            }

            // Full grid every frame (one min-propagation iteration; converges temporally).
            x = WDGlobalGILimits::GridResolutionX;
            y = WDGlobalGILimits::GridResolutionY;
            z = WDGlobalGILimits::GridResolutionZ * cascadeCount;
        }
    } // namespace Render
} // namespace AZ
