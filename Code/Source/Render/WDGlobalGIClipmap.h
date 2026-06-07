/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/std/containers/array.h>

#include <Atom/RHI/Image.h>
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>

#include <WDGlobalGI/WDGlobalGIConfiguration.h>

namespace AZ
{
    namespace RPI
    {
        class ShaderResourceGroup;
    }

    namespace Render
    {
        //! Owns the two persistent, camera-following 3D clipmap textures used by the Dagor GI
        //! solution and computes the per-cascade addressing constants that the compute shaders need.
        //!
        //! - The radiance clipmap stores the lit scene voxels (rgb = radiance, a = opacity/coverage).
        //! - The irradiance clipmap stores a 6-face HL2 Ambient Cube per probe (a = convergence weight).
        //!
        //! Both are addressed toroidally: the physical texel for a world cell is simply
        //! (worldCell & (gridResolution - 1)), which keeps the texture fixed in memory while the
        //! camera moves. Cascade c has cell size baseCellSize * 3^c (nested clipmap).
        class WDGlobalGIClipmap
        {
        public:
            WDGlobalGIClipmap() = default;
            ~WDGlobalGIClipmap() = default;

            //! Per-frame shared shader constants (mirrors the PassSrg constant block in WDGlobalGICommon.azsli).
            struct ShaderConstants
            {
                // xyz = cascade center cell (integer cell index, stored as float), w = cell size in metres.
                AZStd::array<float, 4 * WDGlobalGILimits::MaxCascades> m_cascadeCenterAndCell{};
                uint32_t m_gridResolution[3] = { WDGlobalGILimits::GridResolutionX, WDGlobalGILimits::GridResolutionY, WDGlobalGILimits::GridResolutionZ };
                uint32_t m_cascadeCount = 3;
                float m_sunDirectionIntensity[4] = { 0.0f, -1.0f, 0.0f, 1.0f };
                float m_sunColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
                float m_skyColorIntensity[4] = { 0.3f, 0.44f, 0.62f, 1.0f };
                // Clip->world matrix (row-major), for depth->world reconstruction. Identity by default.
                float m_clipToWorld[16] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
                float m_giIntensity = 1.0f;
                float m_hysteresis = 0.97f;
                uint32_t m_raysPerProbe = 32;
                uint32_t m_maxRaySteps = 24;
                uint32_t m_frameIndex = 0;
                float m_probeUpdateFraction = 0.25f;
                uint32_t m_updateDivisor = 8;
                float m_sceneBlend = 0.5f;
                uint32_t m_debugViewMode = 0;
                float m_leakReduction = 1.0f;
                uint32_t m_clearAll = 0;
                // Previous-frame cascade centre cells (xyz, w=cell size). The toroidal clear pass diffs
                // these against the current centres to find which voxels scrolled into view (anti-ghosting).
                AZStd::array<float, 4 * WDGlobalGILimits::MaxCascades> m_prevCascadeCenterAndCell{};
                uint32_t m_anisotropic = 0;       // 1 = 6-direction anisotropic voxel radiance (#4)
                uint32_t m_useRadianceCache = 0;  // 1 = escaped/long rays reuse cached world irradiance (#8)
                // Last frame's world->clip (row-major) for screen-probe motion reprojection (#2).
                float m_prevWorldToClip[16] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
                float m_cascadeBlend = 1.0f; // 0 = hard cascade boundaries, 1 = blend across them
            };

            //! Allocate the clipmap textures (idempotent).
            void Init();

            //! Release the clipmap textures.
            void Release();

            bool IsInitialized() const { return m_radianceClipmap && m_irradianceClipmap && m_sdfClipmap && m_voxelNormalClipmap && m_albedoClipmap && m_screenProbeAtlas && m_screenProbeSH && m_anisoRadianceClipmap && m_screenProbeAtlasHistory && m_volumetricFroxel && m_screenProbeDepthAtlas && m_screenProbeSHBlurred; }

            //! Recompute the per-cascade centres from the current camera position and refresh the
            //! shader constants from the configuration.
            void Update(
                const AZ::Vector3& cameraPosition,
                const AZ::Matrix4x4& clipToWorld,
                const AZ::Matrix4x4& worldToClip,
                const WDGlobalGIConfiguration& config,
                uint32_t frameIndex,
                bool clearAll);

            //! Mark the clipmaps as needing a cold re-initialisation (see InvalidateClipmaps).
            void Invalidate() { m_invalidated = true; }
            bool ConsumeInvalidated();

            const Data::Instance<RPI::AttachmentImage>& GetRadianceClipmap() const { return m_radianceClipmap; }
            const Data::Instance<RPI::AttachmentImage>& GetIrradianceClipmap() const { return m_irradianceClipmap; }

            //! Signed-distance-field clipmap (manhattan distance-to-surface in cells) used to accelerate
            //! ray marching via sphere tracing. Same layout as the radiance clipmap.
            const Data::Instance<RPI::AttachmentImage>& GetSdfClipmap() const { return m_sdfClipmap; }

            //! Per-voxel surface normal (rgb = normal*0.5+0.5, a = valid), single-layer like the SDF.
            //! Used to attenuate back-facing voxel hits during tracing (light-leak reduction, audit #4).
            const Data::Instance<RPI::AttachmentImage>& GetVoxelNormalClipmap() const { return m_voxelNormalClipmap; }

            //! Per-voxel albedo (rgb), single-layer. Lets the relight pass recompute the lit radiance of
            //! the whole voxel scene when lighting changes, without re-injecting from the camera (audit #5).
            const Data::Instance<RPI::AttachmentImage>& GetAlbedoClipmap() const { return m_albedoClipmap; }

            //! Screen-space probe octahedral radiance atlas (2D, RGBA16F). One OctaRes x OctaRes tile per
            //! screen probe tile; allocated at a fixed max size, the live sub-region is used.
            const Data::Instance<RPI::AttachmentImage>& GetScreenProbeAtlas() const { return m_screenProbeAtlas; }

            //! Per-probe order-2 irradiance SH atlas (2D, RGBA16F): 9 coeff texels per probe in a row.
            const Data::Instance<RPI::AttachmentImage>& GetScreenProbeSH() const { return m_screenProbeSH; }

            //! Per-probe octahedral depth/visibility atlas (same layout as the radiance atlas): rg = mean
            //! hit distance + mean-squared (variance), for DDGI-style Chebyshev visibility weighting. This
            //! is what makes the interpolation leak-free and sharp at contacts.
            const Data::Instance<RPI::AttachmentImage>& GetScreenProbeDepthAtlas() const { return m_screenProbeDepthAtlas; }

            //! Spatially-blurred copy of the irradiance-SH atlas (depth-aware denoise across probes). The
            //! integrate reads this instead of the raw SH so per-probe noise is smoothed out.
            const Data::Instance<RPI::AttachmentImage>& GetScreenProbeSHBlurred() const { return m_screenProbeSHBlurred; }

            //! 6-direction anisotropic voxel radiance (audit #4). Same X/Y as the radiance clipmap, Z
            //! stacked cascade*face like the irradiance clipmap. Inject writes cosine-weighted buckets;
            //! the tracer evaluates them toward the incoming ray so thin walls don't leak light.
            const Data::Instance<RPI::AttachmentImage>& GetAnisoRadianceClipmap() const { return m_anisoRadianceClipmap; }

            //! View-aligned froxel volume holding accumulated in-scattered GI for volumetric fog (#7).
            const Data::Instance<RPI::AttachmentImage>& GetVolumetricFroxel() const { return m_volumetricFroxel; }

            //! Screen-probe atlas ping-pong (audit #2 reprojection): each frame the trace writes the
            //! "current" atlas and reads the "previous" one (reprojected by camera motion); convert /
            //! integrate read the just-written "current". Roles swap by frame parity.
            const Data::Instance<RPI::AttachmentImage>& GetScreenProbeAtlasCurrent(uint32_t frameIndex) const
            {
                return (frameIndex & 1u) ? m_screenProbeAtlasHistory : m_screenProbeAtlas;
            }
            const Data::Instance<RPI::AttachmentImage>& GetScreenProbeAtlasPrev(uint32_t frameIndex) const
            {
                return (frameIndex & 1u) ? m_screenProbeAtlas : m_screenProbeAtlasHistory;
            }

            const ShaderConstants& GetShaderConstants() const { return m_constants; }

            //! Set the debug-view mode constant uploaded to the GI shaders (0 = off).
            void SetDebugViewMode(uint32_t mode) { m_constants.m_debugViewMode = mode; }

            //! Full-resource 3D image-view descriptors for the clipmap attachments (used when declaring
            //! the scope attachments in the GI passes). Centralised here so the gem has a single
            //! definition (important for unity builds).
            static RHI::ImageViewDescriptor GetRadianceViewDescriptor();
            static RHI::ImageViewDescriptor GetIrradianceViewDescriptor();
            static RHI::ImageViewDescriptor GetSdfViewDescriptor();
            static RHI::ImageViewDescriptor GetVoxelNormalViewDescriptor();
            static RHI::ImageViewDescriptor GetAlbedoViewDescriptor();
            static RHI::ImageViewDescriptor GetScreenProbeAtlasViewDescriptor();
            static RHI::ImageViewDescriptor GetScreenProbeShViewDescriptor();
            static RHI::ImageViewDescriptor GetAnisoRadianceViewDescriptor();
            static RHI::ImageViewDescriptor GetVolumetricFroxelViewDescriptor();

            static uint32_t GetRadianceDepth()
            {
                return WDGlobalGILimits::GridResolutionZ * WDGlobalGILimits::MaxCascades;
            }
            static uint32_t GetIrradianceDepth()
            {
                return WDGlobalGILimits::GridResolutionZ * WDGlobalGILimits::MaxCascades * WDGlobalGILimits::AmbientCubeFaceCount;
            }
            //! Anisotropic radiance is stored per face direction, like the irradiance clipmap.
            static uint32_t GetAnisoRadianceDepth()
            {
                return WDGlobalGILimits::GridResolutionZ * WDGlobalGILimits::MaxCascades * WDGlobalGILimits::AmbientCubeFaceCount;
            }

            //! Write the shared constant block (cascade addressing, lighting environment, tuning) onto
            //! a pass shader resource group. Texture views are bound separately by each pass.
            void FillSharedConstants(RPI::ShaderResourceGroup* srg) const;

        private:
            static Data::Instance<RPI::AttachmentImage> CreateClipmapImage(uint32_t depth, RHI::Format format, const char* name);
            static Data::Instance<RPI::AttachmentImage> CreateAtlasImage(uint32_t width, uint32_t height, RHI::Format format, const char* name);
            static Data::Instance<RPI::AttachmentImage> CreateVolumeImage(uint32_t width, uint32_t height, uint32_t depth, RHI::Format format, const char* name);

            Data::Instance<RPI::AttachmentImage> m_radianceClipmap;
            Data::Instance<RPI::AttachmentImage> m_irradianceClipmap;
            Data::Instance<RPI::AttachmentImage> m_sdfClipmap;
            Data::Instance<RPI::AttachmentImage> m_voxelNormalClipmap;
            Data::Instance<RPI::AttachmentImage> m_albedoClipmap;
            Data::Instance<RPI::AttachmentImage> m_screenProbeAtlas;
            Data::Instance<RPI::AttachmentImage> m_screenProbeSH;
            Data::Instance<RPI::AttachmentImage> m_anisoRadianceClipmap;     // 6-direction radiance (#4)
            Data::Instance<RPI::AttachmentImage> m_screenProbeAtlasHistory;  // reprojection ping-pong (#2)
            Data::Instance<RPI::AttachmentImage> m_volumetricFroxel;         // view-aligned froxel GI (#7)
            Data::Instance<RPI::AttachmentImage> m_screenProbeDepthAtlas;    // octahedral depth moments (Chebyshev)
            Data::Instance<RPI::AttachmentImage> m_screenProbeSHBlurred;     // depth-aware spatial denoise of the SH

            ShaderConstants m_constants;
            bool m_invalidated = true;
            AZ::Matrix4x4 m_lastWorldToClip = AZ::Matrix4x4::CreateIdentity(); // for reprojection (#2)

            static constexpr RHI::Format ClipmapFormat = RHI::Format::R16G16B16A16_FLOAT;
            // R32_FLOAT is a guaranteed typed-UAV-load format. The SDF update reads its own UAV
            // (in-place min-propagation), so we avoid R16_FLOAT which needs the optional
            // "typed UAV load additional formats" capability. Memory is trivial (~1.6 MB).
            static constexpr RHI::Format SdfFormat = RHI::Format::R32_FLOAT;
            static constexpr RHI::Format VoxelNormalFormat = RHI::Format::R8G8B8A8_UNORM;
        };
    } // namespace Render
} // namespace AZ
