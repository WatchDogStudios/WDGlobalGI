/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/WDGlobalGIComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        void WDGlobalGIComponentConfig::Reflect(ReflectContext* context)
        {
            // NOTE: the wrapped WDGlobalGIConfiguration (and its edit context with all the sliders) is
            // reflected by WDGlobalGISystemComponent::Reflect, which is always registered (it is a
            // required system component in the same module). We must NOT reflect it again here or the
            // SerializeContext asserts on a duplicate class registration.
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<WDGlobalGIComponentConfig, ComponentConfig>()
                    ->Version(1)
                    ->Field("Settings", &WDGlobalGIComponentConfig::m_settings);
            }
        }
    } // namespace Render
} // namespace AZ
