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
        //! Screen-space probe trace (audit #2/#3). Dispatches one thread per probe octahedral atlas texel,
        //! reconstructs the probe world position from the G-buffer at the tile centre, sphere-traces the
        //! direction through the voxel scene, and temporally accumulates into the octahedral atlas. Only
        //! enabled when the screen-probe layer is active.
        class WDGlobalGIScreenProbeTracePass final
            : public WDGlobalGIComputePass
        {
        public:
            AZ_RPI_PASS(WDGlobalGIScreenProbeTracePass);
            AZ_RTTI(AZ::Render::WDGlobalGIScreenProbeTracePass, "{7081A2B3-C4D5-46E7-F8A9-0B1C2D3E4F50}", WDGlobalGIComputePass);
            AZ_CLASS_ALLOCATOR(WDGlobalGIScreenProbeTracePass, SystemAllocator);

            static RPI::Ptr<WDGlobalGIScreenProbeTracePass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit WDGlobalGIScreenProbeTracePass(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;
            void SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph) override;
            void BindGIResources(const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg) override;
            void GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const override;

            uint32_t m_tilesX = 1;
            uint32_t m_tilesY = 1;
        };
    } // namespace Render
} // namespace AZ
