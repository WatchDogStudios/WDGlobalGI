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
        //! Maintains the signed-distance-field clipmap. Each frame runs one iteration of a min-propagation
        //! (manhattan distance, in cells, to the nearest occupied voxel): occupied voxels are 0, empty
        //! voxels take min(self, neighbour + 1). It converges temporally, matching the GI's update model,
        //! and lets the Propagate / Debug passes sphere-trace instead of single-cell marching.
        class WDGlobalGISdfUpdatePass final
            : public WDGlobalGIComputePass
        {
        public:
            AZ_RPI_PASS(WDGlobalGISdfUpdatePass);
            AZ_RTTI(AZ::Render::WDGlobalGISdfUpdatePass, "{6F708192-A3B4-45C6-D7E8-F90A1B2C3D4E}", WDGlobalGIComputePass);
            AZ_CLASS_ALLOCATOR(WDGlobalGISdfUpdatePass, SystemAllocator);

            static RPI::Ptr<WDGlobalGISdfUpdatePass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit WDGlobalGISdfUpdatePass(const RPI::PassDescriptor& descriptor);

            void SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph) override;
            void BindGIResources(const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg) override;
            void GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const override;
        };
    } // namespace Render
} // namespace AZ
