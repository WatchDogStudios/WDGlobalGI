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
        //! Screen-probe SH spatial denoise. Depth-aware 3x3 blur of the per-probe irradiance SH (between
        //! Convert and Integrate) so the integrate samples smooth, low-noise GI. Runs only when the
        //! screen-probe layer is active.
        class WDGlobalGIScreenProbeBlurPass final
            : public WDGlobalGIComputePass
        {
        public:
            AZ_RPI_PASS(WDGlobalGIScreenProbeBlurPass);
            AZ_RTTI(AZ::Render::WDGlobalGIScreenProbeBlurPass, "{F7081929-3A4B-4C5D-9E6F-7A8B9C0D1E2F}", WDGlobalGIComputePass);
            AZ_CLASS_ALLOCATOR(WDGlobalGIScreenProbeBlurPass, SystemAllocator);

            static RPI::Ptr<WDGlobalGIScreenProbeBlurPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit WDGlobalGIScreenProbeBlurPass(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;
            void SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph) override;
            void BindGIResources(const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg) override;
            void GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const override;

            uint32_t m_tilesX = 1;
            uint32_t m_tilesY = 1;
        };
    } // namespace Render
} // namespace AZ
