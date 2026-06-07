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
        //! Converts each screen probe's octahedral radiance into an order-2 irradiance SH (9 coeffs)
        //! stored in the SH atlas, so the integrate can bilinearly interpolate cheaply. One thread per
        //! probe. Only enabled when the screen-probe layer is active.
        class WDGlobalGIScreenProbeConvertPass final
            : public WDGlobalGIComputePass
        {
        public:
            AZ_RPI_PASS(WDGlobalGIScreenProbeConvertPass);
            AZ_RTTI(AZ::Render::WDGlobalGIScreenProbeConvertPass, "{91A2B3C4-D5E6-47F8-A90B-1C2D3E4F5172}", WDGlobalGIComputePass);
            AZ_CLASS_ALLOCATOR(WDGlobalGIScreenProbeConvertPass, SystemAllocator);

            static RPI::Ptr<WDGlobalGIScreenProbeConvertPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit WDGlobalGIScreenProbeConvertPass(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;
            void SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph) override;
            void BindGIResources(const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg) override;
            void GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const override;

            uint32_t m_tilesX = 1;
            uint32_t m_tilesY = 1;
        };
    } // namespace Render
} // namespace AZ
