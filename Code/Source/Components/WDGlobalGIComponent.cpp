/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/WDGlobalGIComponent.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        WDGlobalGIComponent::WDGlobalGIComponent(const WDGlobalGIComponentConfig& config)
            : BaseClass(config)
        {
        }

        void WDGlobalGIComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<WDGlobalGIComponent, BaseClass>();
            }

            if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty("WDGlobalGIComponentTypeId", BehaviorConstant(Uuid(WDGlobalGIComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
            }
        }
    } // namespace Render
} // namespace AZ
