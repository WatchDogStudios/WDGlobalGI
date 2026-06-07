/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <WDGlobalGIModule.h>
#include <Clients/WDGlobalGISystemComponent.h>
#include <Components/WDGlobalGIComponent.h>

#ifdef WDGLOBALGI_EDITOR
#include <EditorComponents/EditorWDGlobalGIComponent.h>
#endif

namespace AZ
{
    namespace Render
    {
        WDGlobalGIModule::WDGlobalGIModule()
        {
            m_descriptors.insert(m_descriptors.end(),
                {
                    WDGlobalGISystemComponent::CreateDescriptor(),
                    WDGlobalGIComponent::CreateDescriptor(),

#ifdef WDGLOBALGI_EDITOR
                    EditorWDGlobalGIComponent::CreateDescriptor(),
#endif
                });
        }

        AZ::ComponentTypeList WDGlobalGIModule::GetRequiredSystemComponents() const
        {
            return AZ::ComponentTypeList{ azrtti_typeid<WDGlobalGISystemComponent>() };
        }
    } // namespace Render
} // namespace AZ

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AZ::Render::WDGlobalGIModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_WDGlobalGI, AZ::Render::WDGlobalGIModule)
#endif
