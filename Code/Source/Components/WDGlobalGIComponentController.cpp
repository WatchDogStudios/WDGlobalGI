/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/WDGlobalGIComponentController.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        void WDGlobalGIComponentController::Reflect(AZ::ReflectContext* context)
        {
            WDGlobalGIComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<WDGlobalGIComponentController>()
                    ->Version(1)
                    ->Field("Configuration", &WDGlobalGIComponentController::m_configuration);
            }
        }

        void WDGlobalGIComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            // Distinct from the system component's "WDGlobalGIService": this only enforces a single GI
            // level component per entity hierarchy, it does not conflict with the always-present system component.
            provided.push_back(AZ_CRC_CE("WDGlobalGILevelService"));
        }

        void WDGlobalGIComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("WDGlobalGILevelService"));
        }

        void WDGlobalGIComponentController::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
        {
        }

        WDGlobalGIComponentController::WDGlobalGIComponentController(const WDGlobalGIComponentConfig& config)
            : m_configuration(config)
        {
        }

        void WDGlobalGIComponentController::Activate(AZ::EntityId entityId)
        {
            m_entityId = entityId;
            m_featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<WDGlobalGIFeatureProcessorInterface>(entityId);
            OnConfigChanged();
        }

        void WDGlobalGIComponentController::Deactivate()
        {
            m_featureProcessor = nullptr;
            m_entityId = AZ::EntityId();
        }

        void WDGlobalGIComponentController::SetConfiguration(const WDGlobalGIComponentConfig& config)
        {
            m_configuration = config;
            OnConfigChanged();
        }

        const WDGlobalGIComponentConfig& WDGlobalGIComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void WDGlobalGIComponentController::OnConfigChanged()
        {
            if (m_featureProcessor)
            {
                m_featureProcessor->SetConfiguration(m_configuration.m_settings);
            }
        }
    } // namespace Render
} // namespace AZ
