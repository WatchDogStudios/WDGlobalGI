/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Render/Passes/WDGlobalGIComputePass.h>

namespace AZ
{
    namespace Render
    {
        //! Irradiance computation loop (Dagor GI "irradiance map - computation loop").
        //! For a stochastically chosen subset of probes, casts rays through the voxel-radiance clipmap
        //! and accumulates the result into a 6-face HL2 Ambient Cube per probe using a moving average.
        //! NOTE(wdstudiosma) I recommend just using SSRadiance, its more performant, HL2 Solution can be used for baked situations or as a fallback???
        class WDGlobalGIPropagatePass final
            : public WDGlobalGIComputePass
        {
        public:
            AZ_RPI_PASS(WDGlobalGIPropagatePass);
            AZ_RTTI(AZ::Render::WDGlobalGIPropagatePass, "{2B3C4D5E-6F70-4182-93A4-B5C6D7E8F90A}", WDGlobalGIComputePass);
            AZ_CLASS_ALLOCATOR(WDGlobalGIPropagatePass, SystemAllocator);

            static RPI::Ptr<WDGlobalGIPropagatePass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit WDGlobalGIPropagatePass(const RPI::PassDescriptor& descriptor);

            void SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph) override;
            void BindGIResources(const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg) override;
            void GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const override;
        };
    } // namespace Render
} // namespace AZ
