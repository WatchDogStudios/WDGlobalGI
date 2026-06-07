/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <Components/WDGlobalGIComponentConfig.h>
#include <Components/WDGlobalGIComponentConstants.h>
#include <Components/WDGlobalGIComponentController.h>

namespace AZ
{
    namespace Render
    {
        //! Runtime Level component that configures the WDGlobalGI feature processor.
        class WDGlobalGIComponent final
            : public AzFramework::Components::ComponentAdapter<WDGlobalGIComponentController, WDGlobalGIComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<WDGlobalGIComponentController, WDGlobalGIComponentConfig>;
            AZ_COMPONENT(AZ::Render::WDGlobalGIComponent, WDGlobalGIComponentTypeId, BaseClass);

            WDGlobalGIComponent() = default;
            WDGlobalGIComponent(const WDGlobalGIComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
