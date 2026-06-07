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
        //! Volumetric / media GI (audit #7). Fills the view-aligned froxel volume with the accumulated
        //! in-scattered GI (fog lit by the world irradiance), front-to-back. Optional - only runs when
        //! volumetric GI is enabled.
        class WDGlobalGIVolumetricPass final
            : public WDGlobalGIComputePass
        {
        public:
            AZ_RPI_PASS(WDGlobalGIVolumetricPass);
            AZ_RTTI(AZ::Render::WDGlobalGIVolumetricPass, "{D5E6F708-1A2B-4C3D-9E5F-6071829A3B4C}", WDGlobalGIComputePass);
            AZ_CLASS_ALLOCATOR(WDGlobalGIVolumetricPass, SystemAllocator);

            static RPI::Ptr<WDGlobalGIVolumetricPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit WDGlobalGIVolumetricPass(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;
            void SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph) override;
            void BindGIResources(const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg) override;
            void GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const override;
        };
    } // namespace Render
} // namespace AZ
