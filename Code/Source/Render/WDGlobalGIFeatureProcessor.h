/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <WDGlobalGI/WDGlobalGIFeatureProcessorInterface.h>
#include <Render/WDGlobalGIClipmap.h>

namespace AZ
{
    namespace Render
    {
        //! Feature processor implementing the Dagor GI solution.
        //!
        //! Responsibilities:
        //!  - Owns the camera-following voxel-radiance and irradiance clipmaps.
        //!  - Updates the per-cascade clipmap centres each frame from the active view.
        //!  - Injects the GI parent pass (SceneInject -> Propagate -> Apply) into the render pipeline.
        //!  - Exposes the clipmap + configuration to the GI passes.
        class WDGlobalGIFeatureProcessor final
            : public WDGlobalGIFeatureProcessorInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(WDGlobalGIFeatureProcessor, AZ::SystemAllocator)
            AZ_RTTI(AZ::Render::WDGlobalGIFeatureProcessor, "{8E1A0D72-3F4B-4C5D-9E6F-7A8B9C0D1E2F}", AZ::Render::WDGlobalGIFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            WDGlobalGIFeatureProcessor() = default;
            virtual ~WDGlobalGIFeatureProcessor() = default;

            // RPI::FeatureProcessor overrides
            void Activate() override;
            void Deactivate() override;
            void Render(const RenderPacket& packet) override;
            void AddRenderPasses(RPI::RenderPipeline* renderPipeline) override;

            // WDGlobalGIFeatureProcessorInterface overrides
            void SetConfiguration(const WDGlobalGIConfiguration& config) override;
            const WDGlobalGIConfiguration& GetConfiguration() const override { return m_configuration; }
            void SetEnabled(bool enabled) override;
            bool GetEnabled() const override { return m_configuration.m_enabled; }
            void SetQualityLevel(WDGlobalGIQualityLevel qualityLevel) override;
            void InvalidateClipmaps() override;
            void SetDebugViewMode(WDGlobalGIDebugView mode) override { m_debugViewMode = mode; }
            WDGlobalGIDebugView GetDebugViewMode() const override { return m_debugViewMode; }
            void SetShowCascadeBounds(bool show) override { m_showCascadeBounds = show; }
            bool GetShowCascadeBounds() const override { return m_showCascadeBounds; }

            //! Accessors used by the GI compute passes.
            WDGlobalGIClipmap& GetClipmap() { return m_clipmap; }
            const WDGlobalGIClipmap& GetClipmap() const { return m_clipmap; }

            //! True when the GI passes should execute this frame.
            bool ShouldRender() const { return m_configuration.m_enabled && m_clipmap.IsInitialized(); }

            //! Whether the screen-space probe layer is active (replaces the clipmap composite).
            bool GetUseScreenProbes() const { return m_configuration.m_useScreenProbes; }
            float GetScreenProbeTemporal() const { return m_configuration.m_screenProbeTemporal; }
            float GetScreenProbeSpecular() const { return m_configuration.m_screenProbeSpecular; }
            bool GetUseRelight() const { return m_configuration.m_useRelight; }
            bool GetUseAutoSun() const { return m_configuration.m_autoSun; }

            //! Volumetric / media GI (#7) is active (gates the volumetric + composite passes).
            bool GetUseVolumetric() const { return m_configuration.m_useVolumetric; }

            //! True for the first frame after an invalidation (cold start): the SceneInject pass should
            //! seed the voxels with direct light only before the irradiance loop runs.
            bool ConsumeColdStart() { const bool v = m_coldStart; m_coldStart = false; return v; }

        private:
            AZ_DISABLE_COPY_MOVE(WDGlobalGIFeatureProcessor);

            //! Draw cascade-bounds AuxGeom for the active view.
            void DrawDebugBounds(const RenderPacket& packet);

            WDGlobalGIConfiguration m_configuration;
            WDGlobalGIClipmap m_clipmap;
            uint32_t m_frameIndex = 0;
            bool m_coldStart = true;
            WDGlobalGIDebugView m_debugViewMode = WDGlobalGIDebugView::Off;
            bool m_showCascadeBounds = false;
        };
    } // namespace Render
} // namespace AZ
