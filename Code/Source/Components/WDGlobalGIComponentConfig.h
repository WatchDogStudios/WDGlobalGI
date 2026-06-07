/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <WDGlobalGI/WDGlobalGIConfiguration.h>

namespace AZ
{
    namespace Render
    {
        //! Component configuration for the WDGlobalGI level component. Wraps the full GI settings block
        //! so it can be authored on a Level entity and pushed to the feature processor.
        class WDGlobalGIComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_RTTI(WDGlobalGIComponentConfig, "{4B5C6D7E-8F90-4A12-C3D4-E5F60718293A}", ComponentConfig);
            AZ_CLASS_ALLOCATOR(WDGlobalGIComponentConfig, SystemAllocator);

            static void Reflect(ReflectContext* context);

            WDGlobalGIConfiguration m_settings;
        };
    } // namespace Render
} // namespace AZ
