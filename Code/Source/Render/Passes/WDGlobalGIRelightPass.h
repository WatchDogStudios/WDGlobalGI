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
        //! Relights the whole voxel scene from stored albedo + normal: direct sun (SDF-traced voxel
        //! shadow) + indirect ambient cube, overriding the per-voxel radiance (audit #5). Optional -
        //! only runs when relight is enabled. Lets dynamic time-of-day update all voxels, not just the
        //! visible ones the screen-feedback inject refreshes.
        class WDGlobalGIRelightPass final
            : public WDGlobalGIComputePass
        {
        public:
            AZ_RPI_PASS(WDGlobalGIRelightPass);
            AZ_RTTI(AZ::Render::WDGlobalGIRelightPass, "{A2B3C4D5-E6F7-4809-A1B2-C3D4E5F60718}", WDGlobalGIComputePass);
            AZ_CLASS_ALLOCATOR(WDGlobalGIRelightPass, SystemAllocator);

            static RPI::Ptr<WDGlobalGIRelightPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit WDGlobalGIRelightPass(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;
            void SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph) override;
            void BindGIResources(const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg) override;
            void GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const override;

            // Diagnostic: logs once when the directional-light count the auto-sun sees changes.
            int m_loggedDirLightState = -2;
        };
    } // namespace Render
} // namespace AZ
