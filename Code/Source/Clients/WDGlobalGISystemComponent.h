/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#if defined(IMGUI_ENABLED)
#include <ImGuiBus.h>
#endif

namespace AZ
{
    namespace Render
    {
        //! System component that registers the WDGlobalGI feature processor, the GI compute/debug pass
        //! classes, and the pass template mappings. Owns the runtime console variables and draws the
        //! "WD Global GI" ImGui debug tab when ImGui is available.
        class WDGlobalGISystemComponent final
            : public AZ::Component
#if defined(IMGUI_ENABLED)
            , public ImGui::ImGuiUpdateListenerBus::Handler
#endif
        {
        public:
            AZ_COMPONENT(AZ::Render::WDGlobalGISystemComponent, "{4D3C2B1A-9F8E-7D6C-5B4A-3E2D1C0B9A88}");

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            WDGlobalGISystemComponent() = default;
            ~WDGlobalGISystemComponent() override = default;

        protected:
            // AZ::Component overrides
            void Activate() override;
            void Deactivate() override;

#if defined(IMGUI_ENABLED)
            // ImGui::ImGuiUpdateListenerBus::Handler overrides
            void OnImGuiMainMenuUpdate() override;
            void OnImGuiUpdate() override;
#endif

        private:
            void LoadPassTemplateMappings();

            AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler m_loadTemplatesHandler;

#if defined(IMGUI_ENABLED)
            bool m_showImGuiWindow = false;
#endif
        };
    } // namespace Render
} // namespace AZ
