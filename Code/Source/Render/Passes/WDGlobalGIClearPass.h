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
        //! Toroidal clear pass: runs first each frame and zeroes the clipmap voxels that scrolled into
        //! view as the camera moved (or all voxels after an invalidation). Prevents the stale-data
        //! "ghosting" that a toroidally-addressed clipmap otherwise shows when the camera moves.
        class WDGlobalGIClearPass final
            : public WDGlobalGIComputePass
        {
        public:
            AZ_RPI_PASS(WDGlobalGIClearPass);
            AZ_RTTI(AZ::Render::WDGlobalGIClearPass, "{C4D5E6F7-0819-4A23-B4C5-D6E7F8091A23}", WDGlobalGIComputePass);
            AZ_CLASS_ALLOCATOR(WDGlobalGIClearPass, SystemAllocator);

            static RPI::Ptr<WDGlobalGIClearPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit WDGlobalGIClearPass(const RPI::PassDescriptor& descriptor);

            void SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph) override;
            void BindGIResources(const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg) override;
            void GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const override;
        };
    } // namespace Render
} // namespace AZ
