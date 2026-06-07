/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <Components/WDGlobalGIComponent.h>

namespace AZ
{
    namespace Render
    {
        //! Editor wrapper for the WDGlobalGI Level component. Appears under Add Component > Level >
        //! Graphics/Lighting and edits the full GI settings block in the entity inspector.
        class EditorWDGlobalGIComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<WDGlobalGIComponentController, WDGlobalGIComponent, WDGlobalGIComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<WDGlobalGIComponentController, WDGlobalGIComponent, WDGlobalGIComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorWDGlobalGIComponent, EditorWDGlobalGIComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorWDGlobalGIComponent() = default;
            EditorWDGlobalGIComponent(const WDGlobalGIComponentConfig& config);

            //! EditorComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };
    } // namespace Render
} // namespace AZ
