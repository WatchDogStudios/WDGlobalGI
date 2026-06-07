/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <WDGlobalGI/WDGlobalGIConfiguration.h>

namespace AZ
{
    namespace Render
    {
        //! Public interface for the WDGlobalGI (Dagor GI) feature processor.
        //! The feature processor owns the voxel-radiance and irradiance clipmaps that follow the
        //! camera, drives the inject/propagate/apply compute passes, and exposes a small amount of
        //! runtime configuration.
        class WDGlobalGIFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::WDGlobalGIFeatureProcessorInterface, "{2B7E4D11-6A9C-4F03-8D21-7E5A0C3B9F44}", RPI::FeatureProcessor);

            //! Replace the full configuration block.
            virtual void SetConfiguration(const WDGlobalGIConfiguration& config) = 0;

            //! Read back the current configuration.
            virtual const WDGlobalGIConfiguration& GetConfiguration() const = 0;

            //! Enable/disable the whole solution (passes become no-ops while disabled).
            virtual void SetEnabled(bool enabled) = 0;
            virtual bool GetEnabled() const = 0;

            //! Convenience setter for the scalable quality preset.
            virtual void SetQualityLevel(WDGlobalGIQualityLevel qualityLevel) = 0;

            //! Force a full re-initialisation of the clipmaps next frame (e.g. after a camera teleport
            //! or a large destruction event). Solves the "chicken-and-egg" cold start from the paper.
            virtual void InvalidateClipmaps() = 0;

            // --- Debug tooling (driven by the ImGui tab / CVars) ---

            //! Select a debug visualization (Off, voxel-radiance preview, irradiance field, occupancy).
            virtual void SetDebugViewMode(WDGlobalGIDebugView mode) = 0;
            virtual WDGlobalGIDebugView GetDebugViewMode() const = 0;

            //! Draw the clipmap cascade bounds as AuxGeom boxes in the world.
            virtual void SetShowCascadeBounds(bool show) = 0;
            virtual bool GetShowCascadeBounds() const = 0;
        };
    } // namespace Render
} // namespace AZ
