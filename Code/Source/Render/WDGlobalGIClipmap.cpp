/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/WDGlobalGIClipmap.h>

#include <AzCore/Math/MathUtils.h>
#include <cmath>

#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ClearValue.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        RHI::ImageViewDescriptor WDGlobalGIClipmap::GetRadianceViewDescriptor()
        {
            return RHI::ImageViewDescriptor::Create3D(ClipmapFormat, 0, 0, 0, static_cast<uint16_t>(GetRadianceDepth() - 1));
        }

        RHI::ImageViewDescriptor WDGlobalGIClipmap::GetIrradianceViewDescriptor()
        {
            return RHI::ImageViewDescriptor::Create3D(ClipmapFormat, 0, 0, 0, static_cast<uint16_t>(GetIrradianceDepth() - 1));
        }

        RHI::ImageViewDescriptor WDGlobalGIClipmap::GetSdfViewDescriptor()
        {
            return RHI::ImageViewDescriptor::Create3D(SdfFormat, 0, 0, 0, static_cast<uint16_t>(GetRadianceDepth() - 1));
        }

        RHI::ImageViewDescriptor WDGlobalGIClipmap::GetVoxelNormalViewDescriptor()
        {
            return RHI::ImageViewDescriptor::Create3D(VoxelNormalFormat, 0, 0, 0, static_cast<uint16_t>(GetRadianceDepth() - 1));
        }

        RHI::ImageViewDescriptor WDGlobalGIClipmap::GetAlbedoViewDescriptor()
        {
            return RHI::ImageViewDescriptor::Create3D(VoxelNormalFormat, 0, 0, 0, static_cast<uint16_t>(GetRadianceDepth() - 1));
        }

        RHI::ImageViewDescriptor WDGlobalGIClipmap::GetScreenProbeAtlasViewDescriptor()
        {
            return RHI::ImageViewDescriptor::Create(ClipmapFormat, 0, 0);
        }

        RHI::ImageViewDescriptor WDGlobalGIClipmap::GetScreenProbeShViewDescriptor()
        {
            return RHI::ImageViewDescriptor::Create(ClipmapFormat, 0, 0);
        }

        RHI::ImageViewDescriptor WDGlobalGIClipmap::GetAnisoRadianceViewDescriptor()
        {
            return RHI::ImageViewDescriptor::Create3D(ClipmapFormat, 0, 0, 0, static_cast<uint16_t>(GetAnisoRadianceDepth() - 1));
        }

        RHI::ImageViewDescriptor WDGlobalGIClipmap::GetVolumetricFroxelViewDescriptor()
        {
            return RHI::ImageViewDescriptor::Create3D(ClipmapFormat, 0, 0, 0, static_cast<uint16_t>(WDGlobalGILimits::VolumetricFroxelZ - 1));
        }

        Data::Instance<RPI::AttachmentImage> WDGlobalGIClipmap::CreateAtlasImage(uint32_t width, uint32_t height, RHI::Format format, const char* name)
        {
            RHI::ImageDescriptor imageDesc = RHI::ImageDescriptor::Create2D(
                RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyRead, width, height, format);

            RHI::ClearValue clearValue = RHI::ClearValue::CreateVector4Float(0.0f, 0.0f, 0.0f, 0.0f);
            Data::Instance<RPI::AttachmentImagePool> pool = RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
            Data::Instance<RPI::AttachmentImage> image =
                RPI::AttachmentImage::Create(*pool.get(), imageDesc, AZ::Name(name), &clearValue, nullptr);

            AZ_Error("WDGlobalGIClipmap", image, "Failed to create attachment image '%s'", name);
            return image;
        }

        Data::Instance<RPI::AttachmentImage> WDGlobalGIClipmap::CreateVolumeImage(
            uint32_t width, uint32_t height, uint32_t depth, RHI::Format format, const char* name)
        {
            RHI::ImageDescriptor imageDesc = RHI::ImageDescriptor::Create3D(
                RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyRead, width, height, depth, format);

            RHI::ClearValue clearValue = RHI::ClearValue::CreateVector4Float(0.0f, 0.0f, 0.0f, 0.0f);
            Data::Instance<RPI::AttachmentImagePool> pool = RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
            Data::Instance<RPI::AttachmentImage> image =
                RPI::AttachmentImage::Create(*pool.get(), imageDesc, AZ::Name(name), &clearValue, nullptr);

            AZ_Error("WDGlobalGIClipmap", image, "Failed to create volume attachment image '%s'", name);
            return image;
        }

        Data::Instance<RPI::AttachmentImage> WDGlobalGIClipmap::CreateClipmapImage(uint32_t depth, RHI::Format format, const char* name)
        {
            RHI::ImageDescriptor imageDesc = RHI::ImageDescriptor::Create3D(
                RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyRead,
                WDGlobalGILimits::GridResolutionX,
                WDGlobalGILimits::GridResolutionY,
                depth,
                format);

            RHI::ClearValue clearValue = RHI::ClearValue::CreateVector4Float(0.0f, 0.0f, 0.0f, 0.0f);

            Data::Instance<RPI::AttachmentImagePool> pool = RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
            Data::Instance<RPI::AttachmentImage> image =
                RPI::AttachmentImage::Create(*pool.get(), imageDesc, AZ::Name(name), &clearValue, nullptr);

            AZ_Error("WDGlobalGIClipmap", image, "Failed to create clipmap attachment image '%s'", name);
            return image;
        }

        void WDGlobalGIClipmap::Init()
        {
            if (IsInitialized())
            {
                return;
            }

            // Allocate for the maximum number of cascades so a runtime quality change never needs a realloc.
            const uint32_t radianceDepth = WDGlobalGILimits::GridResolutionZ * WDGlobalGILimits::MaxCascades;
            const uint32_t irradianceDepth =
                WDGlobalGILimits::GridResolutionZ * WDGlobalGILimits::MaxCascades * WDGlobalGILimits::AmbientCubeFaceCount;

            m_radianceClipmap = CreateClipmapImage(radianceDepth, ClipmapFormat, "WDGlobalGI_RadianceClipmap");
            m_irradianceClipmap = CreateClipmapImage(irradianceDepth, ClipmapFormat, "WDGlobalGI_IrradianceClipmap");
            m_sdfClipmap = CreateClipmapImage(radianceDepth, SdfFormat, "WDGlobalGI_SdfClipmap");
            m_voxelNormalClipmap = CreateClipmapImage(radianceDepth, VoxelNormalFormat, "WDGlobalGI_VoxelNormalClipmap");
            m_albedoClipmap = CreateClipmapImage(radianceDepth, VoxelNormalFormat, "WDGlobalGI_AlbedoClipmap");
            m_screenProbeAtlas = CreateAtlasImage(
                WDGlobalGILimits::ScreenProbeAtlasMaxWidth, WDGlobalGILimits::ScreenProbeAtlasMaxHeight,
                ClipmapFormat, "WDGlobalGI_ScreenProbeAtlas");
            m_screenProbeSH = CreateAtlasImage(
                WDGlobalGILimits::ScreenProbeShAtlasMaxWidth, WDGlobalGILimits::ScreenProbeShAtlasMaxHeight,
                ClipmapFormat, "WDGlobalGI_ScreenProbeSH");

            // Advanced-quality resources (v1.9). Allocated unconditionally (toggled in the shaders), so a
            // runtime feature toggle never needs a realloc.
            m_anisoRadianceClipmap = CreateClipmapImage(GetAnisoRadianceDepth(), ClipmapFormat, "WDGlobalGI_AnisoRadianceClipmap");
            m_screenProbeAtlasHistory = CreateAtlasImage(
                WDGlobalGILimits::ScreenProbeAtlasMaxWidth, WDGlobalGILimits::ScreenProbeAtlasMaxHeight,
                ClipmapFormat, "WDGlobalGI_ScreenProbeAtlasHistory");
            m_screenProbeDepthAtlas = CreateAtlasImage(
                WDGlobalGILimits::ScreenProbeAtlasMaxWidth, WDGlobalGILimits::ScreenProbeAtlasMaxHeight,
                ClipmapFormat, "WDGlobalGI_ScreenProbeDepthAtlas");
            m_screenProbeSHBlurred = CreateAtlasImage(
                WDGlobalGILimits::ScreenProbeShAtlasMaxWidth, WDGlobalGILimits::ScreenProbeShAtlasMaxHeight,
                ClipmapFormat, "WDGlobalGI_ScreenProbeSHBlurred");
            m_volumetricFroxel = CreateVolumeImage(
                WDGlobalGILimits::VolumetricFroxelX, WDGlobalGILimits::VolumetricFroxelY, WDGlobalGILimits::VolumetricFroxelZ,
                ClipmapFormat, "WDGlobalGI_VolumetricFroxel");

            m_invalidated = true;
        }

        void WDGlobalGIClipmap::Release()
        {
            m_radianceClipmap = nullptr;
            m_irradianceClipmap = nullptr;
            m_sdfClipmap = nullptr;
            m_voxelNormalClipmap = nullptr;
            m_albedoClipmap = nullptr;
            m_screenProbeAtlas = nullptr;
            m_screenProbeSH = nullptr;
            m_anisoRadianceClipmap = nullptr;
            m_screenProbeAtlasHistory = nullptr;
            m_volumetricFroxel = nullptr;
            m_screenProbeDepthAtlas = nullptr;
            m_screenProbeSHBlurred = nullptr;
        }

        bool WDGlobalGIClipmap::ConsumeInvalidated()
        {
            const bool wasInvalidated = m_invalidated;
            m_invalidated = false;
            return wasInvalidated;
        }

        void WDGlobalGIClipmap::Update(
            const AZ::Vector3& cameraPosition,
            const AZ::Matrix4x4& clipToWorld,
            const AZ::Matrix4x4& worldToClip,
            const WDGlobalGIConfiguration& config,
            uint32_t frameIndex,
            bool clearAll)
        {
            const uint32_t cascadeCount = AZ::GetClamp<uint32_t>(config.m_cascadeCount, 1u, WDGlobalGILimits::MaxCascades);
            m_constants.m_cascadeCount = cascadeCount;

            // Snapshot last frame's cascade centres BEFORE recomputing them, so the toroidal clear pass
            // can tell which voxels scrolled into view this frame. m_clearAll forces a full clear (used
            // on invalidation / cell-size change, where every voxel's addressing is stale).
            m_constants.m_prevCascadeCenterAndCell = m_constants.m_cascadeCenterAndCell;
            m_constants.m_clearAll = clearAll ? 1u : 0u;

            // Depth->world reconstruction matrix (stored row-major to match the row_major HLSL declaration).
            clipToWorld.StoreToRowMajorFloat16(m_constants.m_clipToWorld);

            // Screen-probe reprojection (#2): publish LAST frame's world->clip, then remember this frame's
            // for next frame. On the first frame the previous matrix is identity (reprojection just misses).
            m_lastWorldToClip.StoreToRowMajorFloat16(m_constants.m_prevWorldToClip);
            m_lastWorldToClip = worldToClip;

            // Snap each cascade centre to its own cell grid so the toroidal addressing stays stable as the
            // camera moves (the texture only "scrolls" by whole cells).
            float cellSize = config.m_baseCellSize;
            for (uint32_t c = 0; c < WDGlobalGILimits::MaxCascades; ++c)
            {
                const float invCell = 1.0f / cellSize;
                const float centerCellX = std::floor(static_cast<float>(cameraPosition.GetX()) * invCell + 0.5f);
                const float centerCellY = std::floor(static_cast<float>(cameraPosition.GetY()) * invCell + 0.5f);
                const float centerCellZ = std::floor(static_cast<float>(cameraPosition.GetZ()) * invCell + 0.5f);

                m_constants.m_cascadeCenterAndCell[c * 4 + 0] = centerCellX;
                m_constants.m_cascadeCenterAndCell[c * 4 + 1] = centerCellY;
                m_constants.m_cascadeCenterAndCell[c * 4 + 2] = centerCellZ;
                m_constants.m_cascadeCenterAndCell[c * 4 + 3] = cellSize;

                cellSize *= 3.0f; // nested clipmap: cell size triples per cascade.
            }

            // Lighting environment (config-driven in v1).
            AZ::Vector3 sunDir = config.m_sunDirection;
            if (sunDir.GetLengthSq() > 1e-6f)
            {
                sunDir.Normalize();
            }
            m_constants.m_sunDirectionIntensity[0] = static_cast<float>(sunDir.GetX());
            m_constants.m_sunDirectionIntensity[1] = static_cast<float>(sunDir.GetY());
            m_constants.m_sunDirectionIntensity[2] = static_cast<float>(sunDir.GetZ());
            m_constants.m_sunDirectionIntensity[3] = config.m_sunIntensity;

            m_constants.m_sunColor[0] = config.m_sunColor.GetR();
            m_constants.m_sunColor[1] = config.m_sunColor.GetG();
            m_constants.m_sunColor[2] = config.m_sunColor.GetB();
            m_constants.m_sunColor[3] = 1.0f;

            m_constants.m_skyColorIntensity[0] = config.m_skyColor.GetR();
            m_constants.m_skyColorIntensity[1] = config.m_skyColor.GetG();
            m_constants.m_skyColorIntensity[2] = config.m_skyColor.GetB();
            m_constants.m_skyColorIntensity[3] = config.m_skyIntensity;

            // Tuning, scaled by quality level. The dominant cost is the Propagate pass, which casts
            // m_raysPerProbe rays of up to m_maxRaySteps each, for 1/m_updateDivisor of the probes per
            // frame (round-robin over Z slabs). Keep these modest - irradiance accumulates temporally.
            uint32_t raysPerProbe = config.m_raysPerProbe;
            uint32_t maxRaySteps = config.m_maxRaySteps;
            uint32_t updateDivisor = 8;
            switch (config.m_qualityLevel)
            {
            case WDGlobalGIQualityLevel::Low:
                raysPerProbe = 32;
                maxRaySteps = 16;
                updateDivisor = 16; // full refresh every 16 frames
                break;
            case WDGlobalGIQualityLevel::Medium:
                raysPerProbe = 48;
                maxRaySteps = 24;
                updateDivisor = 8;
                break;
            case WDGlobalGIQualityLevel::High:
                raysPerProbe = 64;
                maxRaySteps = 32;
                updateDivisor = 4;
                break;
            default:
                break;
            }

            m_constants.m_giIntensity = config.m_giIntensity;
            m_constants.m_hysteresis = AZ::GetClamp(config.m_hysteresis, 0.0f, 0.999f);
            m_constants.m_raysPerProbe = raysPerProbe;
            m_constants.m_maxRaySteps = maxRaySteps;
            m_constants.m_frameIndex = frameIndex;
            m_constants.m_probeUpdateFraction = AZ::GetClamp(config.m_probeUpdateFraction, 0.0f, 1.0f);
            m_constants.m_updateDivisor = AZ::GetMax(updateDivisor, 1u);
            m_constants.m_sceneBlend = 0.5f;
            m_constants.m_leakReduction = AZ::GetClamp(config.m_leakReduction, 0.0f, 1.0f);
            m_constants.m_anisotropic = config.m_anisotropic ? 1u : 0u;
            m_constants.m_useRadianceCache = config.m_useRadianceCache ? 1u : 0u;
            m_constants.m_cascadeBlend = AZ::GetClamp(config.m_cascadeBlend, 0.0f, 1.0f);
        }

        void WDGlobalGIClipmap::FillSharedConstants(RPI::ShaderResourceGroup* srg) const
        {
            if (!srg)
            {
                return;
            }

            const RHI::ShaderResourceGroupLayout* layout = srg->GetLayout();

            auto setRaw = [&](const char* name, const void* bytes, uint32_t byteCount)
            {
                RHI::ShaderInputConstantIndex index = layout->FindShaderInputConstantIndex(AZ::Name(name));
                if (index.IsValid())
                {
                    srg->SetConstantRaw(index, bytes, byteCount);
                }
            };

            setRaw("m_gridResolution", m_constants.m_gridResolution, sizeof(m_constants.m_gridResolution));
            setRaw("m_cascadeCount", &m_constants.m_cascadeCount, sizeof(m_constants.m_cascadeCount));
            setRaw("m_cascadeCenterAndCell", m_constants.m_cascadeCenterAndCell.data(),
                static_cast<uint32_t>(m_constants.m_cascadeCenterAndCell.size() * sizeof(float)));
            setRaw("m_sunDirectionIntensity", m_constants.m_sunDirectionIntensity, sizeof(m_constants.m_sunDirectionIntensity));
            setRaw("m_sunColor", m_constants.m_sunColor, sizeof(m_constants.m_sunColor));
            setRaw("m_skyColorIntensity", m_constants.m_skyColorIntensity, sizeof(m_constants.m_skyColorIntensity));
            setRaw("m_clipToWorld", m_constants.m_clipToWorld, sizeof(m_constants.m_clipToWorld));
            setRaw("m_giIntensity", &m_constants.m_giIntensity, sizeof(m_constants.m_giIntensity));
            setRaw("m_hysteresis", &m_constants.m_hysteresis, sizeof(m_constants.m_hysteresis));
            setRaw("m_raysPerProbe", &m_constants.m_raysPerProbe, sizeof(m_constants.m_raysPerProbe));
            setRaw("m_maxRaySteps", &m_constants.m_maxRaySteps, sizeof(m_constants.m_maxRaySteps));
            setRaw("m_frameIndex", &m_constants.m_frameIndex, sizeof(m_constants.m_frameIndex));
            setRaw("m_probeUpdateFraction", &m_constants.m_probeUpdateFraction, sizeof(m_constants.m_probeUpdateFraction));
            setRaw("m_updateDivisor", &m_constants.m_updateDivisor, sizeof(m_constants.m_updateDivisor));
            setRaw("m_sceneBlend", &m_constants.m_sceneBlend, sizeof(m_constants.m_sceneBlend));
            setRaw("m_debugViewMode", &m_constants.m_debugViewMode, sizeof(m_constants.m_debugViewMode));
            setRaw("m_leakReduction", &m_constants.m_leakReduction, sizeof(m_constants.m_leakReduction));
            setRaw("m_clearAll", &m_constants.m_clearAll, sizeof(m_constants.m_clearAll));
            setRaw("m_prevCascadeCenterAndCell", m_constants.m_prevCascadeCenterAndCell.data(),
                static_cast<uint32_t>(m_constants.m_prevCascadeCenterAndCell.size() * sizeof(float)));
            setRaw("m_anisotropic", &m_constants.m_anisotropic, sizeof(m_constants.m_anisotropic));
            setRaw("m_useRadianceCache", &m_constants.m_useRadianceCache, sizeof(m_constants.m_useRadianceCache));
            setRaw("m_prevWorldToClip", m_constants.m_prevWorldToClip, sizeof(m_constants.m_prevWorldToClip));
            setRaw("m_cascadeBlend", &m_constants.m_cascadeBlend, sizeof(m_constants.m_cascadeBlend));
        }
    } // namespace Render
} // namespace AZ
