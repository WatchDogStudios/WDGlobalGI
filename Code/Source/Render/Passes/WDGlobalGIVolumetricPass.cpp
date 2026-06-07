/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/Passes/WDGlobalGIVolumetricPass.h>
#include <Render/WDGlobalGIFeatureProcessor.h>

#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<WDGlobalGIVolumetricPass> WDGlobalGIVolumetricPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew WDGlobalGIVolumetricPass(descriptor);
        }

        WDGlobalGIVolumetricPass::WDGlobalGIVolumetricPass(const RPI::PassDescriptor& descriptor)
            : WDGlobalGIComputePass(descriptor, "Shaders/WDGlobalGI/WDGlobalGIVolumetric.azshader")
        {
        }

        bool WDGlobalGIVolumetricPass::IsEnabled() const
        {
            if (!WDGlobalGIComputePass::IsEnabled())
            {
                return false;
            }
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            return fp && fp->GetUseVolumetric();
        }

        void WDGlobalGIVolumetricPass::SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph)
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

            const auto& clipmap = fp->GetClipmap();
            importUse(clipmap.GetIrradianceClipmap(), WDGlobalGIClipmap::GetIrradianceViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            importUse(clipmap.GetVolumetricFroxel(), WDGlobalGIClipmap::GetVolumetricFroxelViewDescriptor(), RHI::ScopeAttachmentAccess::ReadWrite);
        }

        void WDGlobalGIVolumetricPass::BindGIResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg)
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

            const auto& clipmap = fp->GetClipmap();
            setImage("m_irradianceClipmap", clipmap.GetIrradianceClipmap());
            setImage("m_volumetricFroxel", clipmap.GetVolumetricFroxel());

            const WDGlobalGIConfiguration& config = fp->GetConfiguration();
            auto setConstRaw = [&](const char* name, const void* bytes, uint32_t byteCount)
            {
                if (RHI::ShaderInputConstantIndex index = layout->FindShaderInputConstantIndex(AZ::Name(name)); index.IsValid())
                {
                    srg->SetConstantRaw(index, bytes, byteCount);
                }
            };
            setConstRaw("m_volumetricDensity", &config.m_volumetricDensity, sizeof(float));
            setConstRaw("m_volumetricIntensity", &config.m_volumetricIntensity, sizeof(float));
            setConstRaw("m_volumetricMaxDistance", &config.m_volumetricMaxDistance, sizeof(float));
        }

        void WDGlobalGIVolumetricPass::GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const
        {
            x = WDGlobalGILimits::VolumetricFroxelX;
            y = WDGlobalGILimits::VolumetricFroxelY;
            z = 1;
        }
    } // namespace Render
} // namespace AZ
