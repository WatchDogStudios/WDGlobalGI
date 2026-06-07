/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DispatchItem.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        class WDGlobalGIFeatureProcessor;

        //! Shared base for the WDGlobalGI compute passes (SceneInject, Propagate, Apply).
        //!
        //! It owns a single compute shader + pass SRG and runs one dispatch per frame, following the
        //! same RenderPass-derived pattern used by the DiffuseProbeGrid compute passes. Subclasses
        //! provide the clipmap/G-buffer frame-graph dependencies, bind their resource views, and report
        //! the dispatch thread count.
        class WDGlobalGIComputePass
            : public RPI::RenderPass
        {
        public:
            AZ_RTTI(AZ::Render::WDGlobalGIComputePass, "{0F9C7A3E-2D14-4B6A-8E0F-5C7D9A1B3E25}", RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(WDGlobalGIComputePass, SystemAllocator);

        protected:
            WDGlobalGIComputePass(const RPI::PassDescriptor& descriptor, const char* shaderFilePath);
            virtual ~WDGlobalGIComputePass() = default;

            //! Resolve the GI feature processor for this pass's scene (null if absent).
            WDGlobalGIFeatureProcessor* GetFeatureProcessor() const;

            //! Helper: fetch the image view for a connected pipeline input slot from the compile context.
            const RHI::ImageView* GetInputImageView(const RHI::FrameGraphCompileContext& context, const AZ::Name& slotName);

            // --- Subclass hooks --------------------------------------------------------------------

            //! Import the clipmap attachment(s) this pass touches and call UseShaderAttachment on them.
            //! (Connected pipeline slots are already declared by the base.)
            virtual void SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph) = 0;

            //! Bind the pass-specific image views onto the SRG (shared constants are bound by the base).
            virtual void BindGIResources(const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg) = 0;

            //! Report the total number of threads to dispatch in each dimension.
            virtual void GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const = 0;

            // --- RenderPass overrides --------------------------------------------------------------
            bool IsEnabled() const override;
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            Data::Instance<RPI::Shader> m_shader;
            const RHI::PipelineState* m_pipelineState = nullptr;
            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_srgLayout;
            Data::Instance<RPI::ShaderResourceGroup> m_passSrg;
            RHI::DispatchDirect m_dispatchArgs;
            RHI::DispatchItem m_dispatchItem{ RHI::MultiDevice::AllDevices };

        private:
            void LoadShader(const char* shaderFilePath);
        };
    } // namespace Render
} // namespace AZ
