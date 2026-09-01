/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/Passes/WDGlobalGISurfelGridBuildPass.h>
#include <Render/WDGlobalGIFeatureProcessor.h>

#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<WDGlobalGISurfelGridBuildPass> WDGlobalGISurfelGridBuildPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew WDGlobalGISurfelGridBuildPass(descriptor);
        }

        WDGlobalGISurfelGridBuildPass::WDGlobalGISurfelGridBuildPass(const RPI::PassDescriptor& descriptor)
            : WDGlobalGIComputePass(descriptor, "Shaders/WDGlobalGI/WDGlobalGISurfelGridBuild.azshader")
        {
        }

        bool WDGlobalGISurfelGridBuildPass::IsEnabled() const
        {
            if (!WDGlobalGIComputePass::IsEnabled())
            {
                return false;
            }
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            return fp && fp->GetUseScreenProbes() && fp->GetUseSurfels();
        }

        void WDGlobalGISurfelGridBuildPass::SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph)
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

            importUse(fp->GetClipmap().GetSurfelData(), WDGlobalGIClipmap::GetSurfelDataViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            importUse(fp->GetClipmap().GetSurfelGrid(), WDGlobalGIClipmap::GetSurfelGridViewDescriptor(), RHI::ScopeAttachmentAccess::ReadWrite);
        }

        void WDGlobalGISurfelGridBuildPass::BindGIResources(
            [[maybe_unused]] const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg)
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
                if (RHI::ShaderInputImageIndex index = layout->FindShaderInputImageIndex(AZ::Name(name)); index.IsValid())
                {
                    srg->SetImageView(index, image->GetImageView());
                }
            };

            setImage("m_surfelData", fp->GetClipmap().GetSurfelData());
            setImage("m_surfelGrid", fp->GetClipmap().GetSurfelGrid());
        }

        void WDGlobalGISurfelGridBuildPass::GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const
        {
            x = WDGlobalGILimits::SurfelCapacity;
            y = 1;
            z = 1;
        }
    } // namespace Render
} // namespace AZ
