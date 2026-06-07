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

        //! Screen-probe integrate (audit #2). Fullscreen pass that convolves each pixel's screen-probe
        //! octahedral radiance with the pixel cosine hemisphere and add-blends the indirect diffuse into
        //! the lighting target. Replaces the clipmap composite (Apply) when the screen-probe layer is on.
        class WDGlobalGIScreenProbeIntegratePass final
            : public RPI::FullscreenTrianglePass
        {
        public:
            AZ_RPI_PASS(WDGlobalGIScreenProbeIntegratePass);
            AZ_RTTI(AZ::Render::WDGlobalGIScreenProbeIntegratePass, "{8192A3B4-D5E6-47F8-A90B-1C2D3E4F5061}", RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(WDGlobalGIScreenProbeIntegratePass, SystemAllocator);

            static RPI::Ptr<WDGlobalGIScreenProbeIntegratePass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit WDGlobalGIScreenProbeIntegratePass(const RPI::PassDescriptor& descriptor);

            WDGlobalGIFeatureProcessor* GetFeatureProcessor() const;

            bool IsEnabled() const override;
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
        };
    } // namespace Render
} // namespace AZ
