/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/Passes/WDGlobalGISceneInjectPass.h>
#include <Render/WDGlobalGIFeatureProcessor.h>

#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<WDGlobalGISceneInjectPass> WDGlobalGISceneInjectPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew WDGlobalGISceneInjectPass(descriptor);
        }

        WDGlobalGISceneInjectPass::WDGlobalGISceneInjectPass(const RPI::PassDescriptor& descriptor)
            : WDGlobalGIComputePass(descriptor, "Shaders/WDGlobalGI/WDGlobalGISceneInject.azshader")
        {
        }

        void WDGlobalGISceneInjectPass::SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph)
        {
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            if (!fp)
            {
                return;
            }

            const auto& radiance = fp->GetClipmap().GetRadianceClipmap();
            if (!radiance)
            {
                return;
            }

            const auto& irradiance = fp->GetClipmap().GetIrradianceClipmap();

            // Radiance voxels: read-modify-write (moving average injection).
            {
                const RHI::AttachmentId attachmentId = radiance->GetAttachmentId();
                if (!frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentId))
                {
                    frameGraph.GetAttachmentDatabase().ImportImage(attachmentId, radiance->GetRHIImage());
                }

                RHI::ImageScopeAttachmentDescriptor desc;
                desc.m_attachmentId = attachmentId;
                desc.m_imageViewDescriptor = WDGlobalGIClipmap::GetRadianceViewDescriptor();
                desc.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;
                frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::ComputeShader);
            }

            // Voxel surface-normal + albedo clipmaps: read-modify-write.
            auto importRW = [&](const Data::Instance<RPI::AttachmentImage>& image, const RHI::ImageViewDescriptor& viewDesc)
            {
                if (!image)
                {
                    return;
                }
                const RHI::AttachmentId attachmentId = image->GetAttachmentId();
                if (!frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentId))
                {
                    frameGraph.GetAttachmentDatabase().ImportImage(attachmentId, image->GetRHIImage());
                }
                RHI::ImageScopeAttachmentDescriptor desc;
                desc.m_attachmentId = attachmentId;
                desc.m_imageViewDescriptor = viewDesc;
                desc.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;
                frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::ComputeShader);
            };
            importRW(fp->GetClipmap().GetVoxelNormalClipmap(), WDGlobalGIClipmap::GetVoxelNormalViewDescriptor());
            importRW(fp->GetClipmap().GetAlbedoClipmap(), WDGlobalGIClipmap::GetAlbedoViewDescriptor());
            importRW(fp->GetClipmap().GetAnisoRadianceClipmap(), WDGlobalGIClipmap::GetAnisoRadianceViewDescriptor());

            // Irradiance clipmap: read-only (indirect bounce fed back into the voxels).
            if (irradiance)
            {
                const RHI::AttachmentId attachmentId = irradiance->GetAttachmentId();
                if (!frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentId))
                {
                    frameGraph.GetAttachmentDatabase().ImportImage(attachmentId, irradiance->GetRHIImage());
                }

                RHI::ImageScopeAttachmentDescriptor desc;
                desc.m_attachmentId = attachmentId;
                desc.m_imageViewDescriptor = WDGlobalGIClipmap::GetIrradianceViewDescriptor();
                desc.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;
                frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::ComputeShader);
            }
        }

        void WDGlobalGISceneInjectPass::BindGIResources(const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg)
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

            // G-buffer inputs + the real direct diffuse lighting.
            const RHI::ImageView* depthView = GetInputImageView(context, AZ::Name("Depth"));
            setImage("m_depth", depthView);
            setImage("m_normal", GetInputImageView(context, AZ::Name("Normal")));
            setImage("m_albedo", GetInputImageView(context, AZ::Name("Albedo")));
            setImage("m_lighting", GetInputImageView(context, AZ::Name("Lighting")));

            // Clipmaps: radiance (RW), irradiance (read, for the indirect bounce), voxel normal (RW).
            const auto& radiance = fp->GetClipmap().GetRadianceClipmap();
            const auto& irradiance = fp->GetClipmap().GetIrradianceClipmap();
            const auto& voxelNormal = fp->GetClipmap().GetVoxelNormalClipmap();
            const auto& albedo = fp->GetClipmap().GetAlbedoClipmap();
            setImage("m_radianceClipmap", radiance ? radiance->GetImageView() : nullptr);
            setImage("m_irradianceClipmap", irradiance ? irradiance->GetImageView() : nullptr);
            setImage("m_voxelNormalClipmap", voxelNormal ? voxelNormal->GetImageView() : nullptr);
            setImage("m_albedoClipmap", albedo ? albedo->GetImageView() : nullptr);
            const auto& anisoRadiance = fp->GetClipmap().GetAnisoRadianceClipmap();
            setImage("m_anisoRadianceClipmap", anisoRadiance ? anisoRadiance->GetImageView() : nullptr);

            // Cache the dispatch resolution from the depth attachment.
            if (RPI::PassAttachmentBinding* binding = FindAttachmentBinding(AZ::Name("Depth")); binding && binding->GetAttachment())
            {
                const RHI::ImageDescriptor& imageDesc = binding->GetAttachment()->m_descriptor.m_image;
                m_width = imageDesc.m_size.m_width;
                m_height = imageDesc.m_size.m_height;
            }
        }

        void WDGlobalGISceneInjectPass::GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const
        {
            x = AZ::GetMax(m_width, 1u);
            y = AZ::GetMax(m_height, 1u);
            z = 1;
        }
    } // namespace Render
} // namespace AZ
