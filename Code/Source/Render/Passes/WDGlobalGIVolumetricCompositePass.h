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

        //! Volumetric composite (audit #7). Fullscreen pass that samples the froxel volume at each
        //! pixel's view depth and ADD-blends the accumulated in-scattered GI into the lighting target
        //! (One/One). Optional - only runs when volumetric GI is enabled.
        class WDGlobalGIVolumetricCompositePass final
            : public RPI::FullscreenTrianglePass
        {
        public:
            AZ_RPI_PASS(WDGlobalGIVolumetricCompositePass);
            AZ_RTTI(AZ::Render::WDGlobalGIVolumetricCompositePass, "{E6F70819-2A3B-4C5D-9F60-71829A3B4C5D}", RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(WDGlobalGIVolumetricCompositePass, SystemAllocator);

            static RPI::Ptr<WDGlobalGIVolumetricCompositePass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit WDGlobalGIVolumetricCompositePass(const RPI::PassDescriptor& descriptor);

            WDGlobalGIFeatureProcessor* GetFeatureProcessor() const;

            bool IsEnabled() const override;
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
        };
    } // namespace Render
} // namespace AZ
