/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>

namespace AZ
{
    class ReflectContext;

    namespace Render
    {
        //! Quality presets trade voxel-clipmap detail and ray budget for performance.
        //! Mirrors the scalable quality levels described in the Dagor GI talk
        //! ("from low-end PC to ultra-high-end HW support").
        enum class WDGlobalGIQualityLevel : uint8_t
        {
            Low = 0,    //!< Coarse cells, fewer rays, async friendly (~0.7ms target).
            Medium,     //!< Balanced (~1.6ms N-bounce target).
            High,       //!< Finer cells and more rays.
            Count
        };

        //! Debug visualization modes for the GI debug pass / ImGui tab.
        enum class WDGlobalGIDebugView : uint8_t
        {
            Off = 0,
            LitVoxels,       //!< 1: DDA voxel preview, GI-lit radiance, face-shaded (the main voxel view).
            VoxelScene,      //!< 2: stored albedo, face-shaded - the clean voxelized-scene preview (Dagor-style).
            VoxelNormals,    //!< 3: per-voxel surface normals.
            VoxelCascades,   //!< 4: voxels coloured by which clipmap cascade they live in.
            IrradianceField, //!< 5: per-surface GI irradiance field (from the G-buffer).
            TraceSteps,      //!< 6: DDA step-count heat map (trace cost / SDF efficiency).
            Count
        };

        //! Compile-time limits shared by the C++ side and the shaders.
        namespace WDGlobalGILimits
        {
            //! Number of nested clipmap cascades around the camera.
            static constexpr uint32_t MaxCascades = 4;

            //! Voxel resolution of a single cascade (matches the paper's ~64x32x64 irradiance cascade).
            static constexpr uint32_t GridResolutionX = 64;
            static constexpr uint32_t GridResolutionY = 32;
            static constexpr uint32_t GridResolutionZ = 64;

            //! HL2 Ambient Cube basis: 6 directional irradiance values per probe.
            static constexpr uint32_t AmbientCubeFaceCount = 6;

            // Screen-space probes (audit #2/#3). One probe per ScreenProbeTileSize x TileSize screen
            // tile, each storing an OctaRes x OctaRes octahedral radiance map in a 2D atlas. The atlas
            // is allocated once at a fixed maximum (supports up to ~4K) and the live sub-region is used.
            static constexpr uint32_t ScreenProbeTileSize = 16; // must match WDGI_SP_TILE in the shaders
            static constexpr uint32_t ScreenProbeOctaRes = 8;   // must match WDGI_SP_OCTA in the shaders
            static constexpr uint32_t ScreenProbeAtlasMaxWidth = 1920;  // 240 tiles * 8 (4K width / 16)
            static constexpr uint32_t ScreenProbeAtlasMaxHeight = 1080; // 135 tiles * 8 (4K height / 16)

            // Per-probe order-2 irradiance SH (9 coeffs) for cheap bilinear interpolation in the
            // integrate. Atlas stores 9 texels per probe in a row: (tilesX * 9, tilesY).
            static constexpr uint32_t ScreenProbeShCoeffs = 9;
            static constexpr uint32_t ScreenProbeShAtlasMaxWidth = 2160; // 240 tiles * 9 coeffs
            static constexpr uint32_t ScreenProbeShAtlasMaxHeight = 135; // 135 tiles

            // Volumetric GI froxel volume (audit #7): a low-res view-aligned grid. Each froxel samples
            // the world GI for in-scattered indirect light, integrated front-to-back along view depth.
            static constexpr uint32_t VolumetricFroxelX = 160;
            static constexpr uint32_t VolumetricFroxelY = 90;
            static constexpr uint32_t VolumetricFroxelZ = 64;

            // World-space surfel cache (lean surfel GI). Data atlas: 3 RGBA32F texels per surfel
            // (pos+radius / normal+lastSeen / irradiance+count), 1024 surfels per row. Grid: a
            // camera-following 64x64x32-cell window, 8 uint slots per cell (count + 7 indices) =
            // exactly 1024x1024 R32_UINT. Must match the WDGI_SURFEL_* defines in WDGlobalGISurfels.azsli.
            static constexpr uint32_t SurfelCapacity = 65536;
            static constexpr uint32_t SurfelDataStride = 3;
            static constexpr uint32_t SurfelDataAtlasWidth = 1024 * SurfelDataStride;
            static constexpr uint32_t SurfelDataAtlasHeight = SurfelCapacity / 1024;
            static constexpr uint32_t SurfelGridTexSize = 1024;
        }

        //! Runtime-tweakable settings for the global illumination solution.
        struct WDGlobalGIConfiguration
        {
            AZ_TYPE_INFO(WDGlobalGIConfiguration, "{A1F2B3C4-5D6E-7F80-91A2-B3C4D5E6F708}");

            static void Reflect(AZ::ReflectContext* context);

            bool m_enabled = true;
            WDGlobalGIQualityLevel m_qualityLevel = WDGlobalGIQualityLevel::Medium;

            //! Number of active cascades (1..MaxCascades).
            uint32_t m_cascadeCount = 3;

            //! World-space size of a finest-cascade voxel (cascade i cell = m_baseCellSize * 3^i).
            float m_baseCellSize = 0.45f;

            //! Overall multiplier applied to the indirect diffuse contribution.
            float m_giIntensity = 1.0f;

            //! Temporal moving-average factor used when accumulating irradiance (0 = no history, 1 = frozen).
            float m_hysteresis = 0.9f;

            //! Number of rays cast per probe each update (scaled by quality).
            uint32_t m_raysPerProbe = 256;

            //! Maximum number of voxel ray-march steps before falling back to sky.
            uint32_t m_maxRaySteps = 64;

            //! Fraction of screen probes that trace fresh rays each frame (round-robin per tile; the
            //! rest carry validated reprojected history forward). The main screen-probe cost dial.
            float m_probeUpdateFraction = 0.25f;

            //! Use the screen-space probe detail layer (octahedral, per-pixel) instead of compositing
            //! the coarse world irradiance clipmap. The clipmap still runs (multi-bounce feedback +
            //! fallback); screen probes replace the final composite. Off by default (experimental).
            bool m_useScreenProbes = false;

            //! Phase 9: trace the screen-probe rays against the real ray-tracing TLAS
            //! (RayTracingSceneSrg::m_scene) instead of sphere-tracing the SDF/voxel clipmap. Sees real,
            //! unvoxelized geometry - no voxel-resolution cap - at the cost of requiring hardware ray
            //! tracing support and an RT-capable scene. Only takes effect when m_useScreenProbes is also
            //! on; this repoints the screen-probe trace, it does not add a new layer. Off by default: the
            //! SDF path is what every scene already relies on, and hardware RT is not universally
            //! available. WDGlobalGIFeatureProcessor::SetConfiguration() clamps this back to false if the
            //! device reports no ray tracing support, so GI falls back to the SDF path instead of the
            //! screen-probe atlas silently going dark.
            bool m_useHardwareRT = false;

            //! Temporal blend weight for the screen-probe radiance atlas (toward history).
            float m_screenProbeTemporal = 0.9f;

            //! Glossy specular strength from the screen-probe octahedral radiance (0 = diffuse only).
            //! The radiance is scaled by the SpecularF0 G-buffer's per-pixel F0/roughness environment
            //! BRDF, so 1 is the physically-based default (it used to be an unscaled 0.4 flat gain).
            float m_screenProbeSpecular = 1.0f;

            //! Light-leak reduction (audit #4): attenuate voxel hits whose stored surface normal faces
            //! away from the ray. 0 = off (isotropic), 1 = full directional occlusion of back-faces.
            float m_leakReduction = 1.0f;

            //! Relight the whole voxel scene each frame from stored albedo + the config sun (with
            //! SDF-traced voxel shadows) + indirect (audit #5). Lets dynamic time-of-day / sun movement
            //! update ALL voxels, not just the visible ones the screen feedback re-injects. When on, the
            //! relight overrides the per-voxel radiance, so set the sun direction/colour to match the
            //! scene. Off by default (the default real-lighting injection is unaffected).
            bool m_useRelight = false;

            //! When relighting, automatically follow the brightest scene directional light's direction
            //! (read from the DirectionalLightFeatureProcessor's light buffer) instead of m_sunDirection.
            //! Colour/intensity still come from config (scene lights are in lux, a different scale than
            //! the injection path). Falls back to m_sunDirection if no GI-affecting directional light.
            bool m_autoSun = true;

            // --- v1 config-driven lighting environment ---
            // Wiring the real DirectionalLightFeatureProcessor / scene lights is a follow-up;
            // for now the dominant sun and a constant sky are driven from configuration so the
            // solution is self-contained and platform independent.
            AZ::Vector3 m_sunDirection = AZ::Vector3(-0.4f, -0.8f, -0.45f);
            AZ::Color m_sunColor = AZ::Color(1.0f, 0.96f, 0.88f, 1.0f);
            float m_sunIntensity = 4.0f;
            AZ::Color m_skyColor = AZ::Color(0.30f, 0.44f, 0.62f, 1.0f);
            float m_skyIntensity = 1.0f;

            // --- v1.9 advanced quality (all toggleable; default off so the baseline is unchanged) ---

            //! Audit #4: store and sample 6-direction anisotropic voxel radiance. Inject writes
            //! cosine-weighted face buckets; the tracer reads the bucket facing the ray, so a thin wall
            //! lit on one side does not leak light to the other. ~25 MB extra. Off by default.
            bool m_anisotropic = false;

            //! Audit #8: let GI rays that escape the voxel cascades (or march far) reuse the cached world
            //! irradiance clipmap as a far-field / extra-bounce term instead of flat sky. Off by default.
            bool m_useRadianceCache = false;

            //! Global scale on the per-pixel G-buffer roughness that drives the glossy-reflection cone
            //! width (0 = force sharp mirrors, 1 = trust the material roughness). Screen probes only.
            float m_specularRoughness = 1.0f;

            //! Spatial denoise of the screen-probe irradiance SH: a depth-aware 3x3 blur across probes
            //! before integration (0 = off/passthrough, 1 = full blur). The main screen-probe noise fix.
            float m_screenProbeBlur = 1.0f;

            // --- Screen-probe denoiser gap closes: pre-filter, variance-adaptive blend, luminance clamp ---
            // These sit in the trace shaders, upstream of the existing temporal/SH/blur/upscale chain.
            // Each float knob is 0 = off; the luminance clamp has its own bool gate. All only take effect
            // once m_useScreenProbes is already on (default false), matching this feature's opt-in convention.

            //! Firefly clamp applied per-probe, before the temporal blend: any octahedral sample brighter
            //! than (this * the probe's own 8x8 mean luminance) is scaled back down to that ceiling, so a
            //! single bright ray can't survive into the history atlas and smear. 0 = off.
            float m_screenProbeFireflyClamp = 4.0f;

            //! Adaptive temporal weight: an EWMA of each atlas texel's frame-to-frame luminance delta
            //! (persisted in the otherwise-unused .b channel of the depth-moments atlas) scales the
            //! configured Screen Probe Temporal weight down as that texel gets noisier, so converged
            //! probes stay near the full history weight (stop being re-blurred) while noisy ones fall
            //! toward the raw sample (accumulate/converge faster). 0 = off (constant weight, old behaviour).
            float m_screenProbeVarianceScale = 8.0f;

            //! Clamp the post-blend screen-probe radiance to m_screenProbeMaxLuminance, same scheme as
            //! ReflectionScreenSpaceFilter.azsl's SSR luminance clamp (scale the sample down by
            //! maxLuminance/luminance rather than hard-clipping per channel).
            bool m_screenProbeLuminanceClamp = true;
            float m_screenProbeMaxLuminance = 8.0f;

            //! Cascade-blend strength: fade the voxel radiance between adjacent clipmap cascades near
            //! their boundaries to hide the "cascade lines" seam (0 = hard boundaries, 1 = full blend).
            float m_cascadeBlend = 1.0f;

            //! Surfel GI: maintain a persistent world-space surfel cache (spawned from the G-buffer,
            //! irradiance accumulated at fixed world positions) and let it override the screen-probe SH
            //! diffuse where it has coverage. World-anchored accumulation makes the indirect diffuse
            //! temporally rock-stable near the camera; screen probes remain the fallback elsewhere.
            //! Needs m_useScreenProbes. Off by default.
            bool m_useSurfels = false;

            //! Audit #7: volumetric / media GI. Samples the GI into a view-aligned froxel volume for
            //! in-scattered fog light, composited additively into the scene. Off by default.
            bool m_useVolumetric = false;
            float m_volumetricDensity = 0.35f;      //!< Fog thickness (0..1, scales the in-scatter).
            float m_volumetricIntensity = 1.0f;     //!< Multiplier on the in-scattered GI.
            float m_volumetricMaxDistance = 60.0f;  //!< Far distance covered by the froxel volume (m).
        };
    } // namespace Render
} // namespace AZ
