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
        //! Surfel GI: buckets every live surfel into the camera-following cell grid (one thread per
        //! surfel, atomic append into each overlapped cell) so the per-pixel gather and the spawn
        //! coverage test only read a single cell.
        class WDGlobalGISurfelGridBuildPass final
            : public WDGlobalGIComputePass
        {
        public:
            AZ_RPI_PASS(WDGlobalGISurfelGridBuildPass);
            AZ_RTTI(AZ::Render::WDGlobalGISurfelGridBuildPass, "{C8D3E4A2-5F6B-407C-9D0E-1F2A3B4C5D6E}", WDGlobalGIComputePass);
            AZ_CLASS_ALLOCATOR(WDGlobalGISurfelGridBuildPass, SystemAllocator);

            static RPI::Ptr<WDGlobalGISurfelGridBuildPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit WDGlobalGISurfelGridBuildPass(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;
            void SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph) override;
            void BindGIResources(const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg) override;
            void GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const override;
        };
    } // namespace Render
} // namespace AZ
