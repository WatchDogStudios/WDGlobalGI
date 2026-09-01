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
        //! Surfel GI: round-robin per-surfel irradiance update. Traces a small hemisphere ray set
        //! through the voxel scene for 1/m_updateDivisor of the surfels per frame and accumulates the
        //! result at the surfel's fixed world position; also retires long-unseen / out-of-window surfels.
        class WDGlobalGISurfelUpdatePass final
            : public WDGlobalGIComputePass
        {
        public:
            AZ_RPI_PASS(WDGlobalGISurfelUpdatePass);
            AZ_RTTI(AZ::Render::WDGlobalGISurfelUpdatePass, "{EAF5A6C4-7B8D-429E-BF2A-3B4C5D6E7F80}", WDGlobalGIComputePass);
            AZ_CLASS_ALLOCATOR(WDGlobalGISurfelUpdatePass, SystemAllocator);

            static RPI::Ptr<WDGlobalGISurfelUpdatePass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit WDGlobalGISurfelUpdatePass(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;
            void SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph) override;
            void BindGIResources(const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg) override;
            void GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const override;
        };
    } // namespace Render
} // namespace AZ
