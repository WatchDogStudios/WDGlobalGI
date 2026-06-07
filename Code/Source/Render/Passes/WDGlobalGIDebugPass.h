/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>

namespace AZ
{
    namespace Render
    {
        class WDGlobalGIFeatureProcessor;

        //! Debug visualization pass. When a debug view mode is active it OVERWRITES the lighting target
        //! with one of: a camera ray-march of the voxel-radiance clipmap (the "voxel preview"), the
        //! surface irradiance field, or voxel occupancy. Disabled (and free) when the mode is Off.
        class WDGlobalGIDebugPass final
            : public RPI::FullscreenTrianglePass
        {
        public:
            AZ_RPI_PASS(WDGlobalGIDebugPass);
            AZ_RTTI(AZ::Render::WDGlobalGIDebugPass, "{5E6F7081-9203-44B5-C6D7-E8F90A1B2C3D}", RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(WDGlobalGIDebugPass, SystemAllocator);

            static RPI::Ptr<WDGlobalGIDebugPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit WDGlobalGIDebugPass(const RPI::PassDescriptor& descriptor);

            WDGlobalGIFeatureProcessor* GetFeatureProcessor() const;

            // RPI::FullscreenTrianglePass overrides
            bool IsEnabled() const override;
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
        };
    } // namespace Render
} // namespace AZ
