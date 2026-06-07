/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Module.h>

namespace AZ
{
    namespace Render
    {
        class WDGlobalGIModule
            : public AZ::Module
        {
        public:
            AZ_RTTI(WDGlobalGIModule, "{6C0C9E2A-1B3D-4E5F-9A7B-2C8D1E4F6A90}", AZ::Module);
            AZ_CLASS_ALLOCATOR(WDGlobalGIModule, AZ::SystemAllocator);

            WDGlobalGIModule();
            ~WDGlobalGIModule() override = default;

            //! Add required SystemComponents to the SystemEntity.
            AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        };
    } // namespace Render
} // namespace AZ
