/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Clients/WDGlobalGISystemComponent.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>

#include <WDGlobalGI/WDGlobalGIConfiguration.h>
#include <WDGlobalGI/WDGlobalGIFeatureProcessorInterface.h>
#include <Render/WDGlobalGIFeatureProcessor.h>
#include <Render/Passes/WDGlobalGIClearPass.h>
#include <Render/Passes/WDGlobalGISceneInjectPass.h>
#include <Render/Passes/WDGlobalGISdfUpdatePass.h>
#include <Render/Passes/WDGlobalGIRelightPass.h>
#include <Render/Passes/WDGlobalGIPropagatePass.h>
#include <Render/Passes/WDGlobalGIScreenProbeTracePass.h>
#include <Render/Passes/WDGlobalGIScreenProbeConvertPass.h>
#include <Render/Passes/WDGlobalGIScreenProbeBlurPass.h>
#include <Render/Passes/WDGlobalGIApplyPass.h>
#include <Render/Passes/WDGlobalGIScreenProbeIntegratePass.h>
#include <Render/Passes/WDGlobalGIVolumetricPass.h>
#include <Render/Passes/WDGlobalGIVolumetricCompositePass.h>
#include <Render/Passes/WDGlobalGIDebugPass.h>

#if defined(IMGUI_ENABLED)
#include <imgui/imgui.h>
#endif

namespace AZ
{
    namespace Render
    {
        namespace
        {
            //! Resolve the GI feature processor for the primary scene (null if absent).
            WDGlobalGIFeatureProcessorInterface* GetGIFeatureProcessor()
            {
                auto* rpiSystem = RPI::RPISystemInterface::Get();
                if (!rpiSystem)
                {
                    return nullptr;
                }
                if (RPI::Scene* scene = rpiSystem->GetSceneByName(AZ::Name("Main")))
                {
                    return scene->GetFeatureProcessor<WDGlobalGIFeatureProcessorInterface>();
                }
                return nullptr;
            }

            //! Pushes a single setter onto the default scene's feature processor, if present.
            template<typename Fn>
            void ForEachGIFeatureProcessor(Fn&& fn)
            {
                if (auto* fp = GetGIFeatureProcessor())
                {
                    fn(fp);
                }
            }
        } // namespace

        // --- Console variables -------------------------------------------------------------------

        static void OnGiEnabledChanged(const bool& enabled)
        {
            ForEachGIFeatureProcessor([enabled](WDGlobalGIFeatureProcessorInterface* fp) { fp->SetEnabled(enabled); });
        }

        static void OnGiQualityChanged(const int32_t& quality)
        {
            const auto level = static_cast<WDGlobalGIQualityLevel>(
                AZ::GetClamp<int32_t>(quality, 0, static_cast<int32_t>(WDGlobalGIQualityLevel::Count) - 1));
            ForEachGIFeatureProcessor([level](WDGlobalGIFeatureProcessorInterface* fp) { fp->SetQualityLevel(level); });
        }

        static void OnGiIntensityChanged(const float& intensity)
        {
            ForEachGIFeatureProcessor(
                [intensity](WDGlobalGIFeatureProcessorInterface* fp)
                {
                    WDGlobalGIConfiguration config = fp->GetConfiguration();
                    config.m_giIntensity = AZ::GetMax(0.0f, intensity);
                    fp->SetConfiguration(config);
                });
        }

        static void r_wdGlobalGIInvalidate(const AZ::ConsoleCommandContainer&)
        {
            ForEachGIFeatureProcessor([](WDGlobalGIFeatureProcessorInterface* fp) { fp->InvalidateClipmaps(); });
        }

        AZ_CVAR(bool, r_wdGlobalGIEnabled, true, &OnGiEnabledChanged, AZ::ConsoleFunctorFlags::Null,
            "Enable WDGlobalGI (Dagor voxel-clipmap global illumination)");

        AZ_CVAR(int32_t, r_wdGlobalGIQuality, 1, &OnGiQualityChanged, AZ::ConsoleFunctorFlags::Null,
            "WDGlobalGI quality: 0=Low, 1=Medium, 2=High");

        AZ_CVAR(float, r_wdGlobalGIIntensity, 1.0f, &OnGiIntensityChanged, AZ::ConsoleFunctorFlags::Null,
            "WDGlobalGI indirect diffuse intensity multiplier");

        AZ_CONSOLEFREEFUNC(r_wdGlobalGIInvalidate, AZ::ConsoleFunctorFlags::Null,
            "Force WDGlobalGI to re-initialise its clipmaps (use after a camera teleport)");

        // --- Component ---------------------------------------------------------------------------

        void WDGlobalGISystemComponent::Reflect(AZ::ReflectContext* context)
        {
            WDGlobalGIConfiguration::Reflect(context);
            WDGlobalGIFeatureProcessor::Reflect(context);

            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<WDGlobalGISystemComponent, AZ::Component>()
                    ->Version(1);

                if (auto* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<WDGlobalGISystemComponent>(
                        "WD Global Illumination", "Scalable real-time dynamic global illumination (Dagor GI)")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
                }
            }
        }

        void WDGlobalGISystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("WDGlobalGIService"));
        }

        void WDGlobalGISystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("WDGlobalGIService"));
        }

        void WDGlobalGISystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("RPISystem"));
        }

        void WDGlobalGISystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC_CE("AtomFeatureCommonService"));
        }

        void WDGlobalGISystemComponent::Activate()
        {
            // Register the feature processor (exposed through its public interface).
            RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<
                WDGlobalGIFeatureProcessor, WDGlobalGIFeatureProcessorInterface>();

            auto* passSystem = RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "PassSystemInterface is null");

            // Register the GI compute pass classes.
            passSystem->AddPassCreator(AZ::Name("WDGlobalGIClearPass"), &WDGlobalGIClearPass::Create);
            passSystem->AddPassCreator(AZ::Name("WDGlobalGISceneInjectPass"), &WDGlobalGISceneInjectPass::Create);
            passSystem->AddPassCreator(AZ::Name("WDGlobalGISdfUpdatePass"), &WDGlobalGISdfUpdatePass::Create);
            passSystem->AddPassCreator(AZ::Name("WDGlobalGIRelightPass"), &WDGlobalGIRelightPass::Create);
            passSystem->AddPassCreator(AZ::Name("WDGlobalGIPropagatePass"), &WDGlobalGIPropagatePass::Create);
            passSystem->AddPassCreator(AZ::Name("WDGlobalGIScreenProbeTracePass"), &WDGlobalGIScreenProbeTracePass::Create);
            passSystem->AddPassCreator(AZ::Name("WDGlobalGIScreenProbeConvertPass"), &WDGlobalGIScreenProbeConvertPass::Create);
            passSystem->AddPassCreator(AZ::Name("WDGlobalGIScreenProbeBlurPass"), &WDGlobalGIScreenProbeBlurPass::Create);
            passSystem->AddPassCreator(AZ::Name("WDGlobalGIApplyPass"), &WDGlobalGIApplyPass::Create);
            passSystem->AddPassCreator(AZ::Name("WDGlobalGIScreenProbeIntegratePass"), &WDGlobalGIScreenProbeIntegratePass::Create);
            passSystem->AddPassCreator(AZ::Name("WDGlobalGIVolumetricPass"), &WDGlobalGIVolumetricPass::Create);
            passSystem->AddPassCreator(AZ::Name("WDGlobalGIVolumetricCompositePass"), &WDGlobalGIVolumetricCompositePass::Create);
            passSystem->AddPassCreator(AZ::Name("WDGlobalGIDebugPass"), &WDGlobalGIDebugPass::Create);

            // Load the pass template mappings once the pass system is ready.
            m_loadTemplatesHandler = RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler(
                [this]() { this->LoadPassTemplateMappings(); });
            passSystem->ConnectEvent(m_loadTemplatesHandler);

#if defined(IMGUI_ENABLED)
            ImGui::ImGuiUpdateListenerBus::Handler::BusConnect();
#endif
        }

        void WDGlobalGISystemComponent::Deactivate()
        {
#if defined(IMGUI_ENABLED)
            ImGui::ImGuiUpdateListenerBus::Handler::BusDisconnect();
#endif
            m_loadTemplatesHandler.Disconnect();

            RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<WDGlobalGIFeatureProcessor>();
        }

#if defined(IMGUI_ENABLED)
        void WDGlobalGISystemComponent::OnImGuiMainMenuUpdate()
        {
            if (ImGui::BeginMenu("WD Global GI"))
            {
                ImGui::MenuItem("GI Debug Panel", "", &m_showImGuiWindow);
                ImGui::EndMenu();
            }
        }

        void WDGlobalGISystemComponent::OnImGuiUpdate()
        {
            if (!m_showImGuiWindow)
            {
                return;
            }

            ImGui::SetNextWindowSize(ImVec2(400.0f, 460.0f), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("WD Global GI (Dagor GI)", &m_showImGuiWindow))
            {
                WDGlobalGIFeatureProcessorInterface* fp = GetGIFeatureProcessor();
                if (!fp)
                {
                    ImGui::TextColored(ImVec4(1, 0.5f, 0.3f, 1), "No WDGlobalGI feature processor in the active scene.");
                    ImGui::TextWrapped("Add the WDGlobalGI gem to the project and ensure the render pipeline has a 'Forward' pass.");
                    ImGui::End();
                    return;
                }

                WDGlobalGIConfiguration config = fp->GetConfiguration();
                bool cfgChanged = false;

                ImGui::Separator();
                ImGui::TextUnformatted("Solution");
                cfgChanged |= ImGui::Checkbox("Enabled", &config.m_enabled);

                int quality = static_cast<int>(config.m_qualityLevel);
                if (ImGui::Combo("Quality", &quality, "Low\0Medium\0High\0\0"))
                {
                    config.m_qualityLevel = static_cast<WDGlobalGIQualityLevel>(quality);
                    cfgChanged = true;
                }

                cfgChanged |= ImGui::SliderFloat("Intensity", &config.m_giIntensity, 0.0f, 8.0f);
                cfgChanged |= ImGui::SliderFloat("Hysteresis", &config.m_hysteresis, 0.0f, 0.99f);
                cfgChanged |= ImGui::SliderFloat("Leak Reduction", &config.m_leakReduction, 0.0f, 1.0f);
                cfgChanged |= ImGui::SliderFloat("Base Cell Size (m)", &config.m_baseCellSize, 0.1f, 2.0f);

                int cascadeCount = static_cast<int>(config.m_cascadeCount);
                if (ImGui::SliderInt("Cascades", &cascadeCount, 1, 4))
                {
                    config.m_cascadeCount = static_cast<uint32_t>(cascadeCount);
                    cfgChanged = true;
                }

                if (cfgChanged)
                {
                    fp->SetConfiguration(config);
                }

                if (ImGui::Button("Invalidate Clipmaps (teleport)"))
                {
                    fp->InvalidateClipmaps();
                }

                ImGui::Separator();
                ImGui::TextUnformatted("Screen-Space Probes (octahedral, #2/#3)");
                bool spChanged = false;
                spChanged |= ImGui::Checkbox("Use Screen Probes (experimental)", &config.m_useScreenProbes);
                spChanged |= ImGui::SliderFloat("Probe Temporal", &config.m_screenProbeTemporal, 0.0f, 0.98f);
                spChanged |= ImGui::SliderFloat("Glossy Specular", &config.m_screenProbeSpecular, 0.0f, 1.0f);
                if (spChanged)
                {
                    fp->SetConfiguration(config);
                }
                ImGui::TextWrapped("Replaces the coarse clipmap composite with per-pixel octahedral screen probes "
                                   "(traced against the same voxel scene). The clipmap still runs for multi-bounce + fallback.");

                ImGui::Separator();
                ImGui::TextUnformatted("Debug Views");
                int debugView = static_cast<int>(fp->GetDebugViewMode());
                if (ImGui::Combo("View", &debugView,
                        "Off\0Lit Voxels\0Voxel Scene (albedo)\0Voxel Normals\0Cascades\0Irradiance Field\0Trace Steps\0\0"))
                {
                    fp->SetDebugViewMode(static_cast<WDGlobalGIDebugView>(debugView));
                }
                ImGui::TextWrapped("Lit Voxels / Voxel Scene ray-march the voxel clipmap as crisp face-shaded cubes (the "
                                   "Dagor-style preview). Voxel Normals/Cascades inspect the voxel data; Irradiance Field "
                                   "shows the per-surface GI; Trace Steps is a cost heat map (blue cheap -> red expensive).");

                bool showBounds = fp->GetShowCascadeBounds();
                if (ImGui::Checkbox("Show Cascade Bounds (AuxGeom)", &showBounds))
                {
                    fp->SetShowCascadeBounds(showBounds);
                }

                ImGui::Separator();
                ImGui::TextUnformatted("Environment + Dynamic Relight (sun)");
                bool envChanged = false;
                envChanged |= ImGui::Checkbox("Relight Voxels (dynamic time-of-day)", &config.m_useRelight);
                ImGui::TextWrapped("When on, the whole voxel scene is relit each frame from the sun below (with SDF voxel "
                                   "shadows) - dynamic sun/time-of-day updates all voxels, not just visible ones. "
                                   "Off = real-lighting injection (visible voxels only).");
                envChanged |= ImGui::Checkbox("Auto Scene Sun (follow brightest light)", &config.m_autoSun);
                ImGui::TextWrapped("Auto Scene Sun makes relight follow the brightest scene directional light's DIRECTION "
                                   "automatically (colour/intensity stay from the sliders below). Turn it off to drive the "
                                   "direction manually.");
                float sunDir[3] = { config.m_sunDirection.GetX(), config.m_sunDirection.GetY(), config.m_sunDirection.GetZ() };
                if (ImGui::SliderFloat3("Sun Direction", sunDir, -1.0f, 1.0f))
                {
                    config.m_sunDirection.Set(sunDir[0], sunDir[1], sunDir[2]);
                    envChanged = true;
                }
                envChanged |= ImGui::SliderFloat("Sun Intensity", &config.m_sunIntensity, 0.0f, 16.0f);
                envChanged |= ImGui::SliderFloat("Sky Intensity", &config.m_skyIntensity, 0.0f, 8.0f);
                if (envChanged)
                {
                    fp->SetConfiguration(config);
                }

                ImGui::Separator();
                ImGui::TextUnformatted("Advanced Quality (v1.9)");
                bool advChanged = false;
                advChanged |= ImGui::Checkbox("Anisotropic Voxels (no thin-wall leaks)", &config.m_anisotropic);
                advChanged |= ImGui::Checkbox("Radiance Cache (far-field bounce)", &config.m_useRadianceCache);
                advChanged |= ImGui::SliderFloat("Specular Roughness (cone)", &config.m_specularRoughness, 0.0f, 1.0f);
                advChanged |= ImGui::SliderFloat("Screen Probe Blur (denoise)", &config.m_screenProbeBlur, 0.0f, 1.0f);
                advChanged |= ImGui::SliderFloat("Cascade Blend (anti-seam)", &config.m_cascadeBlend, 0.0f, 1.0f);
                advChanged |= ImGui::Checkbox("Volumetric GI (fog in-scatter)", &config.m_useVolumetric);
                advChanged |= ImGui::SliderFloat("Volumetric Density", &config.m_volumetricDensity, 0.0f, 1.0f);
                advChanged |= ImGui::SliderFloat("Volumetric Intensity", &config.m_volumetricIntensity, 0.0f, 4.0f);
                advChanged |= ImGui::SliderFloat("Volumetric Distance (m)", &config.m_volumetricMaxDistance, 5.0f, 200.0f);
                if (advChanged)
                {
                    fp->SetConfiguration(config);
                }
                ImGui::TextWrapped("Anisotropic: 6-direction voxel radiance (~25 MB). Radiance Cache: escaped rays reuse "
                                   "the cached world irradiance. Specular Roughness: glossy cone width. Volumetric: GI "
                                   "scattered through fog (additive). All default off; turn on per scene.");

                ImGui::Separator();
                ImGui::TextUnformatted("Info");
                ImGui::Text("Active cascades: %u", config.m_cascadeCount);
                ImGui::TextWrapped("Note: Inject reads the real scene lighting, so the config sun above is only a fallback. "
                                   "See AUDIT.md for the gap analysis vs Dagor daGI2.");
            }
            ImGui::End();
        }
#endif

        void WDGlobalGISystemComponent::LoadPassTemplateMappings()
        {
            auto* passSystem = RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "PassSystemInterface is null");

            const char* passTemplatesFile = "Passes/WDGlobalGITemplates.azasset";
            passSystem->LoadPassTemplateMappings(passTemplatesFile);
        }
    } // namespace Render
} // namespace AZ
