/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/Passes/WDGlobalGISurfelUpdatePass.h>
#include <Render/WDGlobalGIFeatureProcessor.h>

#include <AzCore/Math/MathUtils.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<WDGlobalGISurfelUpdatePass> WDGlobalGISurfelUpdatePass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew WDGlobalGISurfelUpdatePass(descriptor);
        }

        WDGlobalGISurfelUpdatePass::WDGlobalGISurfelUpdatePass(const RPI::PassDescriptor& descriptor)
            : WDGlobalGIComputePass(descriptor, "Shaders/WDGlobalGI/WDGlobalGISurfelUpdate.azshader")
        {
        }

        bool WDGlobalGISurfelUpdatePass::IsEnabled() const
        {
            if (!WDGlobalGIComputePass::IsEnabled())
            {
                return false;
            }
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            return fp && fp->GetUseScreenProbes() && fp->GetUseSurfels();
        }

        void WDGlobalGISurfelUpdatePass::SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph)
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

            importUse(fp->GetClipmap().GetSurfelData(), WDGlobalGIClipmap::GetSurfelDataViewDescriptor(), RHI::ScopeAttachmentAccess::ReadWrite);
            importUse(fp->GetClipmap().GetRadianceClipmap(), WDGlobalGIClipmap::GetRadianceViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            importUse(fp->GetClipmap().GetSdfClipmap(), WDGlobalGIClipmap::GetSdfViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            importUse(fp->GetClipmap().GetVoxelNormalClipmap(), WDGlobalGIClipmap::GetVoxelNormalViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            importUse(fp->GetClipmap().GetAnisoRadianceClipmap(), WDGlobalGIClipmap::GetAnisoRadianceViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            importUse(fp->GetClipmap().GetIrradianceClipmap(), WDGlobalGIClipmap::GetIrradianceViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
        }

        void WDGlobalGISurfelUpdatePass::BindGIResources(
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
            setImage("m_radianceClipmap", fp->GetClipmap().GetRadianceClipmap());
            setImage("m_sdfClipmap", fp->GetClipmap().GetSdfClipmap());
            setImage("m_voxelNormalClipmap", fp->GetClipmap().GetVoxelNormalClipmap());
            setImage("m_anisoRadianceClipmap", fp->GetClipmap().GetAnisoRadianceClipmap());
            setImage("m_irradianceClipmap", fp->GetClipmap().GetIrradianceClipmap());
        }

        void WDGlobalGISurfelUpdatePass::GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const
        {
            uint32_t divisor = 1;
            if (WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor())
            {
                divisor = AZ::GetMax(fp->GetClipmap().GetShaderConstants().m_updateDivisor, 1u);
            }
            x = (WDGlobalGILimits::SurfelCapacity + divisor - 1) / divisor;
            y = 1;
            z = 1;
        }
    } // namespace Render
} // namespace AZ
