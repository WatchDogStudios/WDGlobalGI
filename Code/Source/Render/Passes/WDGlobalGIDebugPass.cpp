/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/Passes/WDGlobalGIDebugPass.h>
#include <Render/WDGlobalGIFeatureProcessor.h>

#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        namespace
        {
            void ImportClipmapForRead(
                RHI::FrameGraphInterface frameGraph,
                const Data::Instance<RPI::AttachmentImage>& image,
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
                frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::FragmentShader);
            }
        } // namespace

        RPI::Ptr<WDGlobalGIDebugPass> WDGlobalGIDebugPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew WDGlobalGIDebugPass(descriptor);
        }

        WDGlobalGIDebugPass::WDGlobalGIDebugPass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        WDGlobalGIFeatureProcessor* WDGlobalGIDebugPass::GetFeatureProcessor() const
        {
            RPI::Scene* scene = m_pipeline ? m_pipeline->GetScene() : nullptr;
            return scene ? scene->GetFeatureProcessor<WDGlobalGIFeatureProcessor>() : nullptr;
        }

        bool WDGlobalGIDebugPass::IsEnabled() const
        {
            if (!FullscreenTrianglePass::IsEnabled())
            {
                return false;
            }

            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            return fp && fp->ShouldRender() && fp->GetDebugViewMode() != WDGlobalGIDebugView::Off;
        }

        void WDGlobalGIDebugPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            FullscreenTrianglePass::SetupFrameGraphDependencies(frameGraph);

            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            if (!fp)
            {
                return;
            }

            ImportClipmapForRead(frameGraph, fp->GetClipmap().GetRadianceClipmap(), WDGlobalGIClipmap::GetRadianceViewDescriptor());
            ImportClipmapForRead(frameGraph, fp->GetClipmap().GetSdfClipmap(), WDGlobalGIClipmap::GetSdfViewDescriptor());
            ImportClipmapForRead(frameGraph, fp->GetClipmap().GetIrradianceClipmap(), WDGlobalGIClipmap::GetIrradianceViewDescriptor());
            ImportClipmapForRead(frameGraph, fp->GetClipmap().GetAlbedoClipmap(), WDGlobalGIClipmap::GetAlbedoViewDescriptor());
            ImportClipmapForRead(frameGraph, fp->GetClipmap().GetVoxelNormalClipmap(), WDGlobalGIClipmap::GetVoxelNormalViewDescriptor());
        }

        void WDGlobalGIDebugPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            Data::Instance<RPI::ShaderResourceGroup> srg = GetShaderResourceGroup();

            if (fp && srg)
            {
                fp->GetClipmap().FillSharedConstants(srg.get());

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
                setImage("m_irradianceClipmap", fp->GetClipmap().GetIrradianceClipmap());
                setImage("m_albedoClipmap", fp->GetClipmap().GetAlbedoClipmap());
                setImage("m_voxelNormalClipmap", fp->GetClipmap().GetVoxelNormalClipmap());
            }

            FullscreenTrianglePass::CompileResources(context);
        }
    } // namespace Render
} // namespace AZ
