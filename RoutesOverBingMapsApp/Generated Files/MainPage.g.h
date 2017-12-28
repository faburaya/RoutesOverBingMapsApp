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
                namespace Maps {
                    ref class MapControl;
                }
            }
        }
    }
}
namespace Windows {
    namespace UI {
        namespace Xaml {
            namespace Controls {
                ref class SymbolIcon;
                ref class ListView;
                ref class ProgressRing;
                ref class AppBarButton;
                ref class TextBlock;
                ref class AppBarToggleButton;
            }
        }
    }
}

namespace RoutesOverBingMapsApp
{
    [::Windows::Foundation::Metadata::WebHostHidden]
    partial ref class MainPage : public ::Windows::UI::Xaml::Controls::Page, 
        public ::Windows::UI::Xaml::Markup::IComponentConnector,
        public ::Windows::UI::Xaml::Markup::IComponentConnector2
    {
    public:
        void InitializeComponent();
        virtual void Connect(int connectionId, ::Platform::Object^ target);
        virtual ::Windows::UI::Xaml::Markup::IComponentConnector^ GetBindingConnector(int connectionId, ::Platform::Object^ target);
    
    private:
        bool _contentLoaded;
        class MainPage_obj14_Bindings;
        class MainPage_obj18_Bindings;
        class MainPage_obj1_Bindings;
    
        ::XamlBindingInfo::XamlBindings^ Bindings;
        private: ::Windows::UI::Xaml::VisualState^ wideState;
        private: ::Windows::UI::Xaml::VisualState^ narrowState;
        private: ::Windows::UI::Xaml::Controls::Maps::MapControl^ mapControl;
        private: ::Windows::UI::Xaml::Controls::SymbolIcon^ crosshairSymbol;
        private: ::Windows::UI::Xaml::Controls::ListView^ listOfWaypoints;
        private: ::Windows::UI::Xaml::Controls::ProgressRing^ waitingRing;
        private: ::Windows::UI::Xaml::Controls::ListView^ listOfRoutes;
        private: ::Windows::UI::Xaml::Controls::AppBarButton^ clearRoutesButton;
        private: ::Windows::UI::Xaml::Controls::AppBarButton^ clearWaypointsButton;
        private: ::Windows::UI::Xaml::Controls::AppBarButton^ removeWaypointButton;
        private: ::Windows::UI::Xaml::Controls::AppBarButton^ addWaypointButton;
        private: ::Windows::UI::Xaml::Controls::AppBarButton^ findRouteButton;
        private: ::Windows::UI::Xaml::Controls::TextBlock^ serviceTextBlock;
        private: ::Windows::UI::Xaml::Controls::AppBarToggleButton^ useMicrosoftButton;
        private: ::Windows::UI::Xaml::Controls::AppBarToggleButton^ useGoogleButton;
        private: ::Windows::UI::Xaml::Controls::AppBarToggleButton^ useTomtomButton;
        private: ::Windows::UI::Xaml::Controls::AppBarToggleButton^ useAllServicesButton;
    };
}

