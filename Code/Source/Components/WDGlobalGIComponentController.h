/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityId.h>
#include <WDGlobalGI/WDGlobalGIFeatureProcessorInterface.h>
#include <Components/WDGlobalGIComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        //! Drives the WDGlobalGI feature processor from a Level-entity component's configuration. There
        //! is one logical GI volume per scene (camera-following clipmap), so this is a non-placed, global
        //! component - it just forwards the settings block to the feature processor on activate / change.
        class WDGlobalGIComponentController final
        {
        public:
            friend class EditorWDGlobalGIComponent;

            AZ_TYPE_INFO(AZ::Render::WDGlobalGIComponentController, "{3A4B5C6D-7E8F-4901-B2C3-D4E5F6071829}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            WDGlobalGIComponentController() = default;
            WDGlobalGIComponentController(const WDGlobalGIComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const WDGlobalGIComponentConfig& config);
            const WDGlobalGIComponentConfig& GetConfiguration() const;

        private:
            AZ_DISABLE_COPY(WDGlobalGIComponentController);

            void OnConfigChanged();

            WDGlobalGIComponentConfig m_configuration;
            WDGlobalGIFeatureProcessorInterface* m_featureProcessor = nullptr;
            AZ::EntityId m_entityId;
        };
    } // namespace Render
} // namespace AZ
