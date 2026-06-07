/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/Passes/WDGlobalGIPropagatePass.h>
#include <Render/WDGlobalGIFeatureProcessor.h>

#include <AzCore/Math/MathUtils.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>

namespace AZ
{
    namespace Render
    {
        namespace
        {
            void ImportAndUse(
                RHI::FrameGraphInterface frameGraph,
                const RHI::AttachmentId& attachmentId,
                const RHI::Ptr<RHI::Image>& image,
                const RHI::ImageViewDescriptor& viewDesc,
                RHI::ScopeAttachmentAccess access)
            {
                if (!frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentId))
                {
                    frameGraph.GetAttachmentDatabase().ImportImage(attachmentId, image);
                }

                RHI::ImageScopeAttachmentDescriptor desc;
                desc.m_attachmentId = attachmentId;
                desc.m_imageViewDescriptor = viewDesc;
                desc.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;

                frameGraph.UseShaderAttachment(desc, access, RHI::ScopeAttachmentStage::ComputeShader);
            }
        } // namespace

        RPI::Ptr<WDGlobalGIPropagatePass> WDGlobalGIPropagatePass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew WDGlobalGIPropagatePass(descriptor);
        }

        WDGlobalGIPropagatePass::WDGlobalGIPropagatePass(const RPI::PassDescriptor& descriptor)
            : WDGlobalGIComputePass(descriptor, "Shaders/WDGlobalGI/WDGlobalGIPropagate.azshader")
        {
        }

        void WDGlobalGIPropagatePass::SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph)
        {
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            if (!fp)
            {
                return;
            }

            const auto& radiance = fp->GetClipmap().GetRadianceClipmap();
            const auto& irradiance = fp->GetClipmap().GetIrradianceClipmap();
            if (!radiance || !irradiance)
            {
                return;
            }

            // Read the scene voxels...
            ImportAndUse(frameGraph, radiance->GetAttachmentId(), radiance->GetRHIImage(),
                WDGlobalGIClipmap::GetRadianceViewDescriptor(), RHI::ScopeAttachmentAccess::Read);

            // ...read the distance field (for sphere tracing)...
            if (const auto& sdf = fp->GetClipmap().GetSdfClipmap())
            {
                ImportAndUse(frameGraph, sdf->GetAttachmentId(), sdf->GetRHIImage(),
                    WDGlobalGIClipmap::GetSdfViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            }

            // ...read the voxel normals (for leak reduction)...
            if (const auto& voxelNormal = fp->GetClipmap().GetVoxelNormalClipmap())
            {
                ImportAndUse(frameGraph, voxelNormal->GetAttachmentId(), voxelNormal->GetRHIImage(),
                    WDGlobalGIClipmap::GetVoxelNormalViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            }

            // ...read the 6-direction anisotropic radiance (#4)...
            if (const auto& anisoRadiance = fp->GetClipmap().GetAnisoRadianceClipmap())
            {
                ImportAndUse(frameGraph, anisoRadiance->GetAttachmentId(), anisoRadiance->GetRHIImage(),
                    WDGlobalGIClipmap::GetAnisoRadianceViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            }

            // ...write the irradiance ambient cubes (also read back by the radiance cache, #8).
            ImportAndUse(frameGraph, irradiance->GetAttachmentId(), irradiance->GetRHIImage(),
                WDGlobalGIClipmap::GetIrradianceViewDescriptor(), RHI::ScopeAttachmentAccess::ReadWrite);
        }

        void WDGlobalGIPropagatePass::BindGIResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg)
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

            const auto& radiance = fp->GetClipmap().GetRadianceClipmap();
            const auto& irradiance = fp->GetClipmap().GetIrradianceClipmap();
            const auto& sdf = fp->GetClipmap().GetSdfClipmap();
            const auto& voxelNormal = fp->GetClipmap().GetVoxelNormalClipmap();
            const auto& anisoRadiance = fp->GetClipmap().GetAnisoRadianceClipmap();
            setImage("m_radianceClipmap", radiance ? radiance->GetImageView() : nullptr);
            setImage("m_sdfClipmap", sdf ? sdf->GetImageView() : nullptr);
            setImage("m_voxelNormalClipmap", voxelNormal ? voxelNormal->GetImageView() : nullptr);
            setImage("m_anisoRadianceClipmap", anisoRadiance ? anisoRadiance->GetImageView() : nullptr);
            setImage("m_irradianceClipmap", irradiance ? irradiance->GetImageView() : nullptr);
        }

        void WDGlobalGIPropagatePass::GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const
        {
            uint32_t cascadeCount = 1;
            uint32_t divisor = 8;
            if (WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor())
            {
                const auto& constants = fp->GetClipmap().GetShaderConstants();
                cascadeCount = constants.m_cascadeCount;
                divisor = AZ::GetMax(constants.m_updateDivisor, 1u);
            }

            // Only a 1/divisor slice of the probe Z-slabs is updated each frame (round-robin), which is
            // the real cost reduction: we dispatch fewer threads rather than launching all probes and
            // masking most of them (which would still pay full wave cost).
            const uint32_t totalZ = WDGlobalGILimits::GridResolutionZ * cascadeCount;
            x = WDGlobalGILimits::GridResolutionX;
            y = WDGlobalGILimits::GridResolutionY;
            z = (totalZ + divisor - 1) / divisor;
        }
    } // namespace Render
} // namespace AZ
