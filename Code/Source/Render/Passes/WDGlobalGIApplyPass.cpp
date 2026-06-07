/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/Passes/WDGlobalGIApplyPass.h>
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
        RPI::Ptr<WDGlobalGIApplyPass> WDGlobalGIApplyPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew WDGlobalGIApplyPass(descriptor);
        }

        WDGlobalGIApplyPass::WDGlobalGIApplyPass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        WDGlobalGIFeatureProcessor* WDGlobalGIApplyPass::GetFeatureProcessor() const
        {
            RPI::Scene* scene = m_pipeline ? m_pipeline->GetScene() : nullptr;
            return scene ? scene->GetFeatureProcessor<WDGlobalGIFeatureProcessor>() : nullptr;
        }

        bool WDGlobalGIApplyPass::IsEnabled() const
        {
            if (!FullscreenTrianglePass::IsEnabled())
            {
                return false;
            }

            // The screen-probe integrate replaces this clipmap composite when the screen-probe layer is on.
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            return fp && fp->ShouldRender() && !fp->GetUseScreenProbes();
        }

        void WDGlobalGIApplyPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            // Declare the connected pipeline slots (G-buffer reads + the lighting render target).
            FullscreenTrianglePass::SetupFrameGraphDependencies(frameGraph);

            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            if (!fp)
            {
                return;
            }

            const auto& irradiance = fp->GetClipmap().GetIrradianceClipmap();
            if (!irradiance)
            {
                return;
            }

            const RHI::AttachmentId attachmentId = irradiance->GetAttachmentId();
            if (!frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentId))
            {
                frameGraph.GetAttachmentDatabase().ImportImage(attachmentId, irradiance->GetRHIImage());
            }

            RHI::ImageScopeAttachmentDescriptor desc;
            desc.m_attachmentId = attachmentId;
            desc.m_imageViewDescriptor = WDGlobalGIClipmap::GetIrradianceViewDescriptor();
            desc.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;

            frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::FragmentShader);
        }

        void WDGlobalGIApplyPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            Data::Instance<RPI::ShaderResourceGroup> srg = GetShaderResourceGroup();

            if (fp && srg)
            {
                // Shared addressing / lighting / tuning constants.
                fp->GetClipmap().FillSharedConstants(srg.get());

                // Bind the irradiance clipmap (the G-buffer inputs are auto-bound by the base class).
                const auto& irradiance = fp->GetClipmap().GetIrradianceClipmap();
                if (irradiance)
                {
                    RHI::ShaderInputImageIndex index = srg->GetLayout()->FindShaderInputImageIndex(AZ::Name("m_irradianceClipmap"));
                    if (index.IsValid())
                    {
                        srg->SetImageView(index, irradiance->GetImageView());
                    }
                }
            }

            // Base binds the auto-bound G-buffer slots and compiles the SRG (uploading our values too).
            FullscreenTrianglePass::CompileResources(context);
        }
    } // namespace Render
} // namespace AZ
