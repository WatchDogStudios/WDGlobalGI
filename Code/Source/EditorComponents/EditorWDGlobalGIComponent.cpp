/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorComponents/EditorWDGlobalGIComponent.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace AZ
{
    namespace Render
    {
        void EditorWDGlobalGIComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorWDGlobalGIComponent, BaseClass>()
                    ->Version(1);

                if (auto* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorWDGlobalGIComponent>(
                        "WD Global Illumination", "Scalable real-time dynamic global illumination (Dagor voxel-clipmap GI). "
                        "Global / camera-following - its transform is not used, so add it to a Level entity OR any regular "
                        "entity. (On a regular entity the editor avoids the Level-container prefab-override warning.)")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Lighting")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                            // Both menus: "Level" (on the level container) and "Game" (any regular entity).
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level"), AZ_CRC_CE("Game") }))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                    editContext->Class<WDGlobalGIComponentController>(
                        "WDGlobalGIComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &WDGlobalGIComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                    editContext->Class<WDGlobalGIComponentConfig>(
                        "WDGlobalGIComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &WDGlobalGIComponentConfig::m_settings, "Settings", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
                }
            }

            if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty("EditorWDGlobalGIComponentTypeId", BehaviorConstant(Uuid(EditorWDGlobalGIComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorWDGlobalGIComponent::EditorWDGlobalGIComponent(const WDGlobalGIComponentConfig& config)
            : BaseClass(config)
        {
        }

        AZ::u32 EditorWDGlobalGIComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    } // namespace Render
} // namespace AZ
