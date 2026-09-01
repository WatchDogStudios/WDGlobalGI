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
        //! Surfel GI: clears the surfel cell grid, which is rebuilt from the persistent surfel
        //! positions every frame by WDGlobalGISurfelGridBuildPass.
        class WDGlobalGISurfelGridResetPass final
            : public WDGlobalGIComputePass
        {
        public:
            AZ_RPI_PASS(WDGlobalGISurfelGridResetPass);
            AZ_RTTI(AZ::Render::WDGlobalGISurfelGridResetPass, "{B7C2D391-4E5A-4F6B-8C9D-0E1F2A3B4C5D}", WDGlobalGIComputePass);
            AZ_CLASS_ALLOCATOR(WDGlobalGISurfelGridResetPass, SystemAllocator);

            static RPI::Ptr<WDGlobalGISurfelGridResetPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit WDGlobalGISurfelGridResetPass(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;
            void SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph) override;
            void BindGIResources(const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg) override;
            void GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const override;
        };
    } // namespace Render
} // namespace AZ
