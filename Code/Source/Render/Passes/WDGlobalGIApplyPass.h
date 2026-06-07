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

        //! Final composite (Dagor GI "rendering with irradiance map").
        //!
        //! A fullscreen-triangle pass that samples the irradiance clipmap (HL2 Ambient Cube) per pixel,
        //! computes indirect diffuse = (albedo/PI) * irradiance, and ADD-blends it into the deferred
        //! lighting target (the .shader sets a One/One blend). This mirrors the DiffuseProbeGrid
        //! composite. A raster+blend pass is used rather than a compute UAV write because the lighting
        //! target can be multisampled, and MSAA render targets cannot be bound as UAVs.
        class WDGlobalGIApplyPass final
            : public RPI::FullscreenTrianglePass
        {
        public:
            AZ_RPI_PASS(WDGlobalGIApplyPass);
            AZ_RTTI(AZ::Render::WDGlobalGIApplyPass, "{3C4D5E6F-7081-4293-A4B5-C6D7E8F90A1B}", RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(WDGlobalGIApplyPass, SystemAllocator);

            static RPI::Ptr<WDGlobalGIApplyPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit WDGlobalGIApplyPass(const RPI::PassDescriptor& descriptor);

            WDGlobalGIFeatureProcessor* GetFeatureProcessor() const;

            // RPI::FullscreenTrianglePass overrides
            bool IsEnabled() const override;
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
        };
    } // namespace Render
} // namespace AZ
