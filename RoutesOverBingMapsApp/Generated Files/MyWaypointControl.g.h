﻿#pragma once
//------------------------------------------------------------------------------
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//------------------------------------------------------------------------------

#include "XamlBindingInfo.g.h"

namespace Windows {
    namespace UI {
        namespace Xaml {
            ref class VisualState;
        }
    }
}
namespace Windows {
    namespace UI {
        namespace Xaml {
            namespace Controls {
                ref class ProgressRing;
                ref class ComboBox;
                ref class AutoSuggestBox;
                ref class TextBox;
                ref class Button;
            }
        }
    }
}

namespace RoutesOverBingMapsApp
{
    [::Windows::Foundation::Metadata::WebHostHidden]
    partial ref class MyWaypointControl : public ::Windows::UI::Xaml::Controls::UserControl, 
        public ::Windows::UI::Xaml::Markup::IComponentConnector,
        public ::Windows::UI::Xaml::Markup::IComponentConnector2
    {
    public:
        void InitializeComponent();
        virtual void Connect(int connectionId, ::Platform::Object^ target);
        virtual ::Windows::UI::Xaml::Markup::IComponentConnector^ GetBindingConnector(int connectionId, ::Platform::Object^ target);
    
    private:
        bool _contentLoaded;
        class MyWaypointControl_obj11_Bindings;
        class MyWaypointControl_obj1_Bindings;
    
        ::XamlBindingInfo::XamlBindings^ Bindings;
        private: ::Windows::UI::Xaml::VisualState^ wideState;
        private: ::Windows::UI::Xaml::VisualState^ narrowState;
        private: ::Windows::UI::Xaml::Controls::ProgressRing^ waitingRing;
        private: ::Windows::UI::Xaml::Controls::ComboBox^ wayptInputTypeComboBox;
        private: ::Windows::UI::Xaml::Controls::AutoSuggestBox^ addressASBox;
        private: ::Windows::UI::Xaml::Controls::TextBox^ latitudeTextBox;
        private: ::Windows::UI::Xaml::Controls::TextBox^ longitudeTextBox;
        private: ::Windows::UI::Xaml::Controls::Button^ locateButton;
    };
}

