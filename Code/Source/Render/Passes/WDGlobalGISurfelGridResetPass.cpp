/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/Passes/WDGlobalGISurfelGridResetPass.h>
#include <Render/WDGlobalGIFeatureProcessor.h>

#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<WDGlobalGISurfelGridResetPass> WDGlobalGISurfelGridResetPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew WDGlobalGISurfelGridResetPass(descriptor);
        }

        WDGlobalGISurfelGridResetPass::WDGlobalGISurfelGridResetPass(const RPI::PassDescriptor& descriptor)
            : WDGlobalGIComputePass(descriptor, "Shaders/WDGlobalGI/WDGlobalGISurfelGridReset.azshader")
        {
        }

        bool WDGlobalGISurfelGridResetPass::IsEnabled() const
        {
            if (!WDGlobalGIComputePass::IsEnabled())
            {
                return false;
            }
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            return fp && fp->GetUseScreenProbes() && fp->GetUseSurfels();
        }

        void WDGlobalGISurfelGridResetPass::SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph)
        {
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            if (!fp)
            {
                return;
            }
            const auto& grid = fp->GetClipmap().GetSurfelGrid();
            if (!grid)
            {
                return;
            }
            const RHI::AttachmentId id = grid->GetAttachmentId();
            if (!frameGraph.GetAttachmentDatabase().IsAttachmentValid(id))
            {
                frameGraph.GetAttachmentDatabase().ImportImage(id, grid->GetRHIImage());
            }
            RHI::ImageScopeAttachmentDescriptor desc;
            desc.m_attachmentId = id;
            desc.m_imageViewDescriptor = WDGlobalGIClipmap::GetSurfelGridViewDescriptor();
            desc.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;
            frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::ComputeShader);
        }

        void WDGlobalGISurfelGridResetPass::BindGIResources(
            [[maybe_unused]] const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg)
        {
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            if (!fp)
            {
                return;
            }
            const auto& grid = fp->GetClipmap().GetSurfelGrid();
            if (RHI::ShaderInputImageIndex index = srg->GetLayout()->FindShaderInputImageIndex(AZ::Name("m_surfelGrid"));
                index.IsValid() && grid)
            {
                srg->SetImageView(index, grid->GetImageView());
            }
        }

        void WDGlobalGISurfelGridResetPass::GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const
        {
            x = WDGlobalGILimits::SurfelGridTexSize;
            y = WDGlobalGILimits::SurfelGridTexSize;
            z = 1;
        }
    } // namespace Render
} // namespace AZ
