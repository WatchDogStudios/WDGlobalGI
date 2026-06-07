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
        //! Scene feedback loop (Dagor GI "scene feedback").
        //! Relights G-buffer pixels with direct light + the current irradiance clipmap and writes the
        //! result into the voxel-radiance clipmap with a moving average. This both provides multiple
        //! light bounces and fixes voxelization issues (thin walls), exactly as described in the talk.
        class WDGlobalGISceneInjectPass final
            : public WDGlobalGIComputePass
        {
        public:
            AZ_RPI_PASS(WDGlobalGISceneInjectPass);
            AZ_RTTI(AZ::Render::WDGlobalGISceneInjectPass, "{1A2B3C4D-5E6F-4071-8293-A4B5C6D7E8F9}", WDGlobalGIComputePass);
            AZ_CLASS_ALLOCATOR(WDGlobalGISceneInjectPass, SystemAllocator);

            static RPI::Ptr<WDGlobalGISceneInjectPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit WDGlobalGISceneInjectPass(const RPI::PassDescriptor& descriptor);

            void SetupGIFrameGraph(RHI::FrameGraphInterface frameGraph) override;
            void BindGIResources(const RHI::FrameGraphCompileContext& context, RPI::ShaderResourceGroup* srg) override;
            void GetDispatchThreadCount(uint32_t& x, uint32_t& y, uint32_t& z) const override;

            uint32_t m_width = 0;
            uint32_t m_height = 0;
        };
    } // namespace Render
} // namespace AZ
