/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/Passes/WDGlobalGIRelightPass.h>
#include <Render/WDGlobalGIFeatureProcessor.h>

#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/Feature/CoreLights/DirectionalLightFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<WDGlobalGIRelightPass> WDGlobalGIRelightPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew WDGlobalGIRelightPass(descriptor);
        }

        WDGlobalGIRelightPass::WDGlobalGIRelightPass(const RPI::PassDescriptor& descriptor)
            : WDGlobalGIComputePass(descriptor, "Shaders/WDGlobalGI/WDGlobalGIRelight.azshader")
        {
        }

        bool WDGlobalGIRelightPass::IsEnabled() const
        {
            if (!WDGlobalGIComputePass::IsEnabled())
            {
                return false;
            }
            WDGlobalGIFeatureProcessor* fp = GetFeatureProcessor();
            return fp && fp->GetUseRelight();
        }

        void WDGlobalGIRelightPass::SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph)
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
            importUse(clipmap.GetAlbedoClipmap(), WDGlobalGIClipmap::GetAlbedoViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            importUse(clipmap.GetVoxelNormalClipmap(), WDGlobalGIClipmap::GetVoxelNormalViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            importUse(clipmap.GetIrradianceClipmap(), WDGlobalGIClipmap::GetIrradianceViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            importUse(clipmap.GetSdfClipmap(), WDGlobalGIClipmap::GetSdfViewDescriptor(), RHI::ScopeAttachmentAccess::Read);
            importUse(clipmap.GetRadianceClipmap(), WDGlobalGIClipmap::GetRadianceViewDescriptor(), RHI::ScopeAttachmentAccess::ReadWrite);
        }

        void WDGlobalGIRelightPass::BindGIResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg)
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
            setImage("m_albedoClipmap", clipmap.GetAlbedoClipmap());
            setImage("m_voxelNormalClipmap", clipmap.GetVoxelNormalClipmap());
            setImage("m_irradianceClipmap", clipmap.GetIrradianceClipmap());
            setImage("m_sdfClipmap", clipmap.GetSdfClipmap());
            setImage("m_radianceClipmap", clipmap.GetRadianceClipmap());

            // Auto scene sun: bind the scene's directional-light buffer + count so the shader can follow
            // the brightest light's direction. The directional-light feature processor exists in any
            // pipeline that has the Forward lighting pass (which WDGlobalGI requires), so its buffer is
            // valid - this mirrors how RayTracingFeatureProcessor binds the same light buffer.
            RPI::Scene* scene = m_pipeline ? m_pipeline->GetScene() : nullptr;
            auto* dirLightFp = scene ? scene->GetFeatureProcessor<DirectionalLightFeatureProcessorInterface>() : nullptr;

            uint32_t lightCount = 0;
            if (dirLightFp)
            {
                if (const Data::Instance<RPI::Buffer> lightBuffer = dirLightFp->GetLightBuffer())
                {
                    const RHI::ShaderInputBufferIndex bufferIndex = layout->FindShaderInputBufferIndex(AZ::Name("m_directionalLights"));
                    if (bufferIndex.IsValid())
                    {
                        srg->SetBufferView(bufferIndex, lightBuffer->GetBufferView());
                        lightCount = dirLightFp->GetLightCount();
                    }
                }
            }

            const uint32_t autoSun = (fp->GetUseAutoSun() && lightCount > 0) ? 1u : 0u;

            // Diagnostic: report whether the auto-sun actually found the scene's directional light(s).
            // Logs only when the situation changes (e.g. when a sun is added/removed), so it's not spammy.
            const int dirLightState = static_cast<int>(lightCount) * 2 + (dirLightFp ? 1 : 0);
            if (dirLightState != m_loggedDirLightState)
            {
                m_loggedDirLightState = dirLightState;
                if (!dirLightFp)
                {
                    AZ_Warning("WDGlobalGI", false, "Relight auto-sun: no DirectionalLightFeatureProcessor in the scene.");
                }
                else
                {
                    AZ_TracePrintf("WDGlobalGI", "Relight auto-sun: %u directional light(s) found in the scene (auto-sun %s).\n",
                        lightCount, (autoSun != 0) ? "active" : "falling back to the config sun");
                }
            }

            auto setConstRaw = [&](const char* name, const void* bytes, uint32_t byteCount)
            {
                const RHI::ShaderInputConstantIndex index = layout->FindShaderInputConstantIndex(AZ::Name(name));
                if (index.IsValid())
                {
                    srg->SetConstantRaw(index, bytes, byteCount);
                }
            };
            setConstRaw("m_autoSun", &autoSun, sizeof(autoSun));
            setConstRaw("m_sceneDirLightCount", &lightCount, sizeof(lightCount));
        }

        void WDGlobalGIRelightPass::GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const
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
