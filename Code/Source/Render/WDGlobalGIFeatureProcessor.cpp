/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/WDGlobalGIFeatureProcessor.h>

#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>

namespace AZ
{
    namespace Render
    {
        void WDGlobalGIConfiguration::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<WDGlobalGIConfiguration>()
                    ->Version(1)
                    ->Field("Enabled", &WDGlobalGIConfiguration::m_enabled)
                    ->Field("QualityLevel", &WDGlobalGIConfiguration::m_qualityLevel)
                    ->Field("CascadeCount", &WDGlobalGIConfiguration::m_cascadeCount)
                    ->Field("BaseCellSize", &WDGlobalGIConfiguration::m_baseCellSize)
                    ->Field("GIIntensity", &WDGlobalGIConfiguration::m_giIntensity)
                    ->Field("Hysteresis", &WDGlobalGIConfiguration::m_hysteresis)
                    ->Field("RaysPerProbe", &WDGlobalGIConfiguration::m_raysPerProbe)
                    ->Field("MaxRaySteps", &WDGlobalGIConfiguration::m_maxRaySteps)
                    ->Field("ProbeUpdateFraction", &WDGlobalGIConfiguration::m_probeUpdateFraction)
                    ->Field("UseScreenProbes", &WDGlobalGIConfiguration::m_useScreenProbes)
                    ->Field("ScreenProbeTemporal", &WDGlobalGIConfiguration::m_screenProbeTemporal)
                    ->Field("ScreenProbeSpecular", &WDGlobalGIConfiguration::m_screenProbeSpecular)
                    ->Field("LeakReduction", &WDGlobalGIConfiguration::m_leakReduction)
                    ->Field("UseRelight", &WDGlobalGIConfiguration::m_useRelight)
                    ->Field("AutoSun", &WDGlobalGIConfiguration::m_autoSun)
                    ->Field("SunDirection", &WDGlobalGIConfiguration::m_sunDirection)
                    ->Field("SunColor", &WDGlobalGIConfiguration::m_sunColor)
                    ->Field("SunIntensity", &WDGlobalGIConfiguration::m_sunIntensity)
                    ->Field("SkyColor", &WDGlobalGIConfiguration::m_skyColor)
                    ->Field("SkyIntensity", &WDGlobalGIConfiguration::m_skyIntensity)
                    ->Field("Anisotropic", &WDGlobalGIConfiguration::m_anisotropic)
                    ->Field("UseRadianceCache", &WDGlobalGIConfiguration::m_useRadianceCache)
                    ->Field("SpecularRoughness", &WDGlobalGIConfiguration::m_specularRoughness)
                    ->Field("ScreenProbeBlur", &WDGlobalGIConfiguration::m_screenProbeBlur)
                    ->Field("CascadeBlend", &WDGlobalGIConfiguration::m_cascadeBlend)
                    ->Field("UseVolumetric", &WDGlobalGIConfiguration::m_useVolumetric)
                    ->Field("VolumetricDensity", &WDGlobalGIConfiguration::m_volumetricDensity)
                    ->Field("VolumetricIntensity", &WDGlobalGIConfiguration::m_volumetricIntensity)
                    ->Field("VolumetricMaxDistance", &WDGlobalGIConfiguration::m_volumetricMaxDistance);

                if (auto* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<WDGlobalGIConfiguration>("WD Global Illumination Settings", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &WDGlobalGIConfiguration::m_enabled, "Enabled", "Enable the GI solution")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &WDGlobalGIConfiguration::m_qualityLevel, "Quality", "Scalable quality preset")
                            ->EnumAttribute(WDGlobalGIQualityLevel::Low, "Low")
                            ->EnumAttribute(WDGlobalGIQualityLevel::Medium, "Medium")
                            ->EnumAttribute(WDGlobalGIQualityLevel::High, "High")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &WDGlobalGIConfiguration::m_cascadeCount, "Cascades", "Number of nested clipmap cascades")
                            ->Attribute(AZ::Edit::Attributes::Min, 1)
                            ->Attribute(AZ::Edit::Attributes::Max, static_cast<int>(WDGlobalGILimits::MaxCascades))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &WDGlobalGIConfiguration::m_baseCellSize, "Base Cell Size", "World-space size of a finest-cascade voxel (metres)")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.05f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &WDGlobalGIConfiguration::m_giIntensity, "GI Intensity", "Indirect diffuse multiplier")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 8.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &WDGlobalGIConfiguration::m_hysteresis, "Hysteresis", "Temporal moving-average factor")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 0.999f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &WDGlobalGIConfiguration::m_raysPerProbe, "Rays Per Probe", "Base ray budget per probe update")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &WDGlobalGIConfiguration::m_maxRaySteps, "Max Ray Steps", "Voxel ray-march step budget")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &WDGlobalGIConfiguration::m_probeUpdateFraction, "Probe Update Fraction", "Fraction of probes refreshed per frame")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &WDGlobalGIConfiguration::m_useScreenProbes, "Use Screen Probes", "Per-pixel octahedral screen-space probes (sharper, Lumen/daGI2-style) instead of the coarse 6-value ambient-cube clipmap composite. The single biggest quality jump - recommended.")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &WDGlobalGIConfiguration::m_screenProbeTemporal, "Screen Probe Temporal", "Temporal accumulation for the screen probes (higher = smoother but slower to respond).")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 0.98f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &WDGlobalGIConfiguration::m_screenProbeSpecular, "Screen Probe Specular", "Glossy reflection strength from the screen probes (0 = diffuse only).")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &WDGlobalGIConfiguration::m_useRelight, "Relight Voxels", "Relight the whole voxel scene from albedo + sun each frame (dynamic time-of-day, with SDF voxel shadows). Off = real-lighting injection of visible voxels only.")
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &WDGlobalGIConfiguration::m_autoSun, "Auto Scene Sun", "When relighting, follow the brightest scene directional light's direction automatically (instead of the Sun Direction below).")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &WDGlobalGIConfiguration::m_sunDirection, "Sun Direction", "Dominant light direction (toward the surface). Used when Auto Scene Sun is off.")
                        ->DataElement(AZ::Edit::UIHandlers::Color, &WDGlobalGIConfiguration::m_sunColor, "Sun Color", "Dominant light colour")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &WDGlobalGIConfiguration::m_sunIntensity, "Sun Intensity", "Dominant light intensity")
                        ->DataElement(AZ::Edit::UIHandlers::Color, &WDGlobalGIConfiguration::m_skyColor, "Sky Color", "Ambient sky colour")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &WDGlobalGIConfiguration::m_skyIntensity, "Sky Intensity", "Ambient sky intensity")
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &WDGlobalGIConfiguration::m_anisotropic, "Anisotropic Voxels", "Store/sample 6-direction radiance so thin walls don't leak light (~25 MB extra).")
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &WDGlobalGIConfiguration::m_useRadianceCache, "Radiance Cache", "Escaped/long GI rays reuse the cached world irradiance as far-field light (extra bounce).")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &WDGlobalGIConfiguration::m_specularRoughness, "Specular Roughness", "Glossy reflection cone width (0 = sharp mirror, 1 = blurry). Screen probes only.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &WDGlobalGIConfiguration::m_screenProbeBlur, "Screen Probe Blur", "Depth-aware spatial denoise of the screen-probe GI (0 = off, 1 = full). The main noise fix.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &WDGlobalGIConfiguration::m_cascadeBlend, "Cascade Blend", "Blend voxel radiance across clipmap cascade boundaries to hide cascade-line seams (0 = hard, 1 = full).")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &WDGlobalGIConfiguration::m_useVolumetric, "Volumetric GI", "In-scatter indirect light into a view-aligned fog volume (additive).")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &WDGlobalGIConfiguration::m_volumetricDensity, "Volumetric Density", "Fog thickness.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &WDGlobalGIConfiguration::m_volumetricIntensity, "Volumetric Intensity", "Multiplier on the in-scattered GI.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &WDGlobalGIConfiguration::m_volumetricMaxDistance, "Volumetric Max Distance", "Far distance covered by the froxel volume (metres).");
                }
            }
        }

        void WDGlobalGIFeatureProcessor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<WDGlobalGIFeatureProcessor, FeatureProcessor>()
                    ->Version(1);
            }
        }

        void WDGlobalGIFeatureProcessor::Activate()
        {
            // The clipmap textures are allocated lazily on the first Render() once the RHI is live.
            EnableSceneNotification();
        }

        void WDGlobalGIFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();
            m_clipmap.Release();
        }

        void WDGlobalGIFeatureProcessor::Render(const RenderPacket& packet)
        {
            if (!m_configuration.m_enabled)
            {
                return;
            }

            // Lazily allocate the clipmap textures.
            if (!m_clipmap.IsInitialized())
            {
                m_clipmap.Init();
                if (!m_clipmap.IsInitialized())
                {
                    return;
                }
            }

            // Drive the clipmap centres from the primary view's camera position, and provide the
            // clip->world matrix so the GI compute shaders can reconstruct world position from depth
            // without needing a ViewSrg binding.
            AZ::Vector3 cameraPosition = AZ::Vector3::CreateZero();
            AZ::Matrix4x4 clipToWorld = AZ::Matrix4x4::CreateIdentity();
            AZ::Matrix4x4 worldToClip = AZ::Matrix4x4::CreateIdentity();
            if (!packet.m_views.empty() && packet.m_views.front())
            {
                const auto& view = packet.m_views.front();
                cameraPosition = view->GetViewToWorldMatrix().GetTranslation();
                worldToClip = view->GetWorldToClipMatrix();
                clipToWorld = worldToClip.GetInverseFull();
            }

            const bool invalidated = m_clipmap.ConsumeInvalidated();
            if (invalidated)
            {
                m_coldStart = true;
            }

            // On an invalidation (teleport / cell-size change) the clear pass wipes every voxel so no
            // stale data survives. Otherwise it only clears the slabs that scrolled into view this frame.
            m_clipmap.Update(cameraPosition, clipToWorld, worldToClip, m_configuration, m_frameIndex, invalidated);
            m_clipmap.SetDebugViewMode(static_cast<uint32_t>(m_debugViewMode));
            ++m_frameIndex;

            if (m_showCascadeBounds)
            {
                DrawDebugBounds(packet);
            }
        }

        void WDGlobalGIFeatureProcessor::DrawDebugBounds([[maybe_unused]] const RenderPacket& packet)
        {
            auto auxGeom = RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(GetParentScene());
            if (!auxGeom)
            {
                return;
            }

            const WDGlobalGIClipmap::ShaderConstants& c = m_clipmap.GetShaderConstants();
            const AZ::Color cascadeColors[WDGlobalGILimits::MaxCascades] = {
                AZ::Color(1.0f, 0.2f, 0.2f, 1.0f),
                AZ::Color(0.2f, 1.0f, 0.2f, 1.0f),
                AZ::Color(0.3f, 0.5f, 1.0f, 1.0f),
                AZ::Color(1.0f, 1.0f, 0.2f, 1.0f),
            };

            for (uint32_t i = 0; i < c.m_cascadeCount; ++i)
            {
                const float cellSize = c.m_cascadeCenterAndCell[i * 4 + 3];
                const AZ::Vector3 center(
                    c.m_cascadeCenterAndCell[i * 4 + 0] * cellSize,
                    c.m_cascadeCenterAndCell[i * 4 + 1] * cellSize,
                    c.m_cascadeCenterAndCell[i * 4 + 2] * cellSize);
                const AZ::Vector3 halfExtent(
                    0.5f * WDGlobalGILimits::GridResolutionX * cellSize,
                    0.5f * WDGlobalGILimits::GridResolutionY * cellSize,
                    0.5f * WDGlobalGILimits::GridResolutionZ * cellSize);

                const AZ::Aabb aabb = AZ::Aabb::CreateCenterHalfExtents(center, halfExtent);
                auxGeom->DrawAabb(aabb, cascadeColors[i], RPI::AuxGeomDraw::DrawStyle::Line);
            }
        }

        void WDGlobalGIFeatureProcessor::AddRenderPasses(RPI::RenderPipeline* renderPipeline)
        {
            // Only inject into pipelines that expose a Forward (deferred lighting) pass with the
            // G-buffer outputs we consume.
            RPI::PassFilter forwardFilter = RPI::PassFilter::CreateWithPassName(AZ::Name("Forward"), renderPipeline);
            RPI::Pass* forwardPass = RPI::PassSystemInterface::Get()->FindFirstPass(forwardFilter);
            if (!forwardPass)
            {
                return;
            }

            // Skip if we have already injected the parent into this pipeline.
            RPI::PassFilter existingFilter = RPI::PassFilter::CreateWithPassName(AZ::Name("WDGlobalGIPass"), renderPipeline);
            if (RPI::PassSystemInterface::Get()->FindFirstPass(existingFilter))
            {
                return;
            }

            // Insert the GI parent immediately after the Forward pass so the irradiance is composited
            // into the diffuse lighting target before post processing.
            RPI::AddPassRequestToRenderPipeline(
                renderPipeline,
                "Passes/WDGlobalGIParentPassRequest.azasset",
                "Forward",
                /*beforeReferencePass=*/false);
        }

        void WDGlobalGIFeatureProcessor::SetConfiguration(const WDGlobalGIConfiguration& config)
        {
            // A change to the clipmap geometry (cell size / cascade count) invalidates every voxel's
            // toroidal addressing, so force a full clear next frame rather than mixing old/new-scale data.
            const bool geometryChanged =
                config.m_baseCellSize != m_configuration.m_baseCellSize ||
                config.m_cascadeCount != m_configuration.m_cascadeCount;

            m_configuration = config;
            m_configuration.m_cascadeCount =
                AZ::GetClamp<uint32_t>(m_configuration.m_cascadeCount, 1u, WDGlobalGILimits::MaxCascades);

            if (geometryChanged)
            {
                m_clipmap.Invalidate();
            }
        }

        void WDGlobalGIFeatureProcessor::SetEnabled(bool enabled)
        {
            m_configuration.m_enabled = enabled;
        }

        void WDGlobalGIFeatureProcessor::SetQualityLevel(WDGlobalGIQualityLevel qualityLevel)
        {
            if (qualityLevel < WDGlobalGIQualityLevel::Count)
            {
                m_configuration.m_qualityLevel = qualityLevel;
            }
        }

        void WDGlobalGIFeatureProcessor::InvalidateClipmaps()
        {
            m_clipmap.Invalidate();
        }
    } // namespace Render
} // namespace AZ
