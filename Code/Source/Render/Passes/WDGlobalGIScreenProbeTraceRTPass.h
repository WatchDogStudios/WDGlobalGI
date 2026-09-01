/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/Feature/RayTracing/RayTracingPass.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        class WDGlobalGIFeatureProcessor;

        //! Phase 9: hardware ray-traced variant of the screen-probe trace. Same octahedral atlas and
        //! temporal/reprojection scheme as WDGlobalGIScreenProbeTracePass, but each probe ray is traced
        //! against the real TLAS (RayTracingSceneSrg::m_scene, via WDGlobalGIScreenProbeTraceRT.azsl +
        //! ...ClosestHit/...Miss) instead of sphere-traced through the SDF/voxel clipmap. Selected by
        //! WDGlobalGIConfiguration::m_useHardwareRT and mutually exclusive with the compute pass (see its
        //! IsEnabled()); both write the same clipmap atlas images.
        //!
        //! Atom's RayTracingPass only auto-binds the one SRG it reflects from the raygen shader's own
        //! SRG_RayTracingGlobal slot (see RayTracingPass::CreatePipelineState/CompileResources) - that
        //! slot's layout is fixed by Reflections (forced via RayCones.azsli's include chain) and this pass
        //! never touches it. Everything this pass actually needs - the atlas, its history, the shared GI
        //! constants - lives in its own SRG_PerPass SRG instead, built and bound here exactly like
        //! WDGlobalGIComputePass binds its PassSrg, then handed to the base class's dispatch by pushing it
        //! into the protected m_rayTracingSRGsToBind list.
        class WDGlobalGIScreenProbeTraceRTPass final
            : public RayTracingPass
        {
        public:
            AZ_RPI_PASS(WDGlobalGIScreenProbeTraceRTPass);
            AZ_RTTI(AZ::Render::WDGlobalGIScreenProbeTraceRTPass, "{6B7C8D9E-1F2A-4B3C-8D4E-5F6A7B8C9D01}", RayTracingPass);
            AZ_CLASS_ALLOCATOR(WDGlobalGIScreenProbeTraceRTPass, SystemAllocator);

            static RPI::Ptr<WDGlobalGIScreenProbeTraceRTPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit WDGlobalGIScreenProbeTraceRTPass(const RPI::PassDescriptor& descriptor);

            WDGlobalGIFeatureProcessor* GetFeatureProcessor() const;

            bool IsEnabled() const override;
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_srgLayout;
            Data::Instance<RPI::ShaderResourceGroup> m_passSrg;
        };
    } // namespace Render
} // namespace AZ
