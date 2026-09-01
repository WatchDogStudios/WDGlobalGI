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
        //! Surfel GI: screen-driven surfel spawning (one thread per screen tile). Samples a jittered
        //! G-buffer pixel, tests surfel coverage through the cell grid, allocates a new surfel from the
        //! ring cursor where coverage is missing, and refreshes last-seen on covering surfels.
        class WDGlobalGISurfelSpawnPass final
            : public WDGlobalGIComputePass
        {
        public:
            AZ_RPI_PASS(WDGlobalGISurfelSpawnPass);
            AZ_RTTI(AZ::Render::WDGlobalGISurfelSpawnPass, "{D9E4F5B3-6A7C-418D-AE1F-2A3B4C5D6E7F}", WDGlobalGIComputePass);
            AZ_CLASS_ALLOCATOR(WDGlobalGISurfelSpawnPass, SystemAllocator);

            static RPI::Ptr<WDGlobalGISurfelSpawnPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit WDGlobalGISurfelSpawnPass(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;
            void SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph) override;
            void BindGIResources(const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg) override;
            void GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const override;

            uint32_t m_tilesX = 1;
            uint32_t m_tilesY = 1;
        };
    } // namespace Render
} // namespace AZ
