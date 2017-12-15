﻿//------------------------------------------------------------------------------
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//------------------------------------------------------------------------------
#include "pch.h"

#pragma warning(push)
#pragma warning(disable: 4100) // unreferenced formal parameter

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BINDING_DEBUG_OUTPUT
extern "C" __declspec(dllimport) int __stdcall IsDebuggerPresent();
#endif

#include "MainPage.xaml.h"

void ::RoutesOverBingMapsApp::MainPage::InitializeComponent()
{
    if (_contentLoaded)
    {
        return;
    }
    _contentLoaded = true;
    ::Windows::Foundation::Uri^ resourceLocator = ref new ::Windows::Foundation::Uri(L"ms-appx:///MainPage.xaml");
    ::Windows::UI::Xaml::Application::LoadComponent(this, resourceLocator, ::Windows::UI::Xaml::Controls::Primitives::ComponentResourceLocation::Application);
}


/// <summary>
/// Auto generated class for compiled bindings.
/// </summary>
class RoutesOverBingMapsApp::MainPage::MainPage_obj12_Bindings 
    : public ::XamlBindingInfo::ReferenceTypeXamlBindings<::RoutesOverBingMapsApp::Waypoint>
{
public:
    MainPage_obj12_Bindings()
    {
    }

    void Connect(int __connectionId, ::Platform::Object^ __target)
    {
        switch(__connectionId)
        {
            case 13:
                this->obj13 = safe_cast<::RoutesOverBingMapsApp::MyWaypointControl^>(__target);
                break;
        }
    }

    void ResetTemplate()
    {
    }

    int ProcessBindings(::Windows::UI::Xaml::Controls::ContainerContentChangingEventArgs^ args)
    {
        int nextPhase = -1;
        switch(args->Phase)
        {
            case 0:
                nextPhase = -1;
                this->SetDataRoot(static_cast<::RoutesOverBingMapsApp::Waypoint^>(args->Item));
                if (this->_dataContextChangedToken.Value != 0)
                {
                    safe_cast<::Windows::UI::Xaml::FrameworkElement^>(args->ItemContainer->ContentTemplateRoot)->DataContextChanged -= this->_dataContextChangedToken;
                    this->_dataContextChangedToken.Value = 0;
                }
                this->_isInitialized = true;
                break;
        }
        this->Update_((::RoutesOverBingMapsApp::Waypoint^) args->Item, (1 << args->Phase));
        return nextPhase;
    }
private:
    // Fields for each control that has bindings.
    ::RoutesOverBingMapsApp::MyWaypointControl^ obj13;

    // Update methods for each path node used in binding steps.
    void Update_(::RoutesOverBingMapsApp::Waypoint^ obj, int phase)
    {
        if (obj != nullptr)
        {
            if ((phase & (NOT_PHASED | (1 << 0))) != 0)
            {
                this->Update_Self(obj->Self, phase);
            }
        }
    }
    void Update_Self(::RoutesOverBingMapsApp::Waypoint^ obj, int phase)
    {
        if((phase & ((1 << 0) | NOT_PHASED )) != 0)
        {
            ::XamlBindingInfo::XamlBindingSetters::Set_RoutesOverBingMapsApp_MyWaypointControl_ViewModel(this->obj13, obj, nullptr);
        }
    }
};

/// <summary>
/// Auto generated class for compiled bindings.
/// </summary>
class RoutesOverBingMapsApp::MainPage::MainPage_obj1_Bindings 
    : public ::XamlBindingInfo::ReferenceTypeXamlBindings<::RoutesOverBingMapsApp::MainPage>
{
public:
    MainPage_obj1_Bindings()
    {
    }

    void Connect(int __connectionId, ::Platform::Object^ __target)
    {
        switch(__connectionId)
        {
            case 7:
                this->obj7 = safe_cast<::Windows::UI::Xaml::Controls::ListView^>(__target);
                break;
        }
    }
private:
    // Fields for each control that has bindings.
    ::Windows::UI::Xaml::Controls::ListView^ obj7;

    // Update methods for each path node used in binding steps.
    void Update_(::RoutesOverBingMapsApp::MainPage^ obj, int phase)
    {
        if (obj != nullptr)
        {
            if ((phase & (NOT_PHASED | (1 << 0))) != 0)
            {
                this->Update_ViewModel(obj->ViewModel, phase);
            }
        }
    }
    void Update_ViewModel(::RoutesOverBingMapsApp::AppViewModel^ obj, int phase)
    {
        if (obj != nullptr)
        {
            if ((phase & (NOT_PHASED | (1 << 0))) != 0)
            {
                this->Update_ViewModel_Waypoints(obj->Waypoints, phase);
            }
        }
    }
    void Update_ViewModel_Waypoints(::Windows::Foundation::Collections::IObservableVector<::RoutesOverBingMapsApp::Waypoint^>^ obj, int phase)
    {
        if((phase & ((1 << 0) | NOT_PHASED )) != 0)
        {
            ::XamlBindingInfo::XamlBindingSetters::Set_Windows_UI_Xaml_Controls_ItemsControl_ItemsSource(this->obj7, obj, nullptr);
        }
    }
};

void ::RoutesOverBingMapsApp::MainPage::Connect(int __connectionId, ::Platform::Object^ __target)
{
    switch (__connectionId)
    {
    case 2:
        {
            this->wideState = safe_cast<::Windows::UI::Xaml::VisualState^>(__target);
        }
        break;
    case 3:
        {
            this->narrowState = safe_cast<::Windows::UI::Xaml::VisualState^>(__target);
        }
        break;
    case 4:
        {
            this->mapControl = safe_cast<::Windows::UI::Xaml::Controls::Maps::MapControl^>(__target);
        }
        break;
    case 5:
        {
            this->crosshairSymbol = safe_cast<::Windows::UI::Xaml::Controls::SymbolIcon^>(__target);
        }
        break;
    case 6:
        {
            this->waitingRing = safe_cast<::Windows::UI::Xaml::Controls::ProgressRing^>(__target);
        }
        break;
    case 7:
        {
            this->listOfWaypoints = safe_cast<::Windows::UI::Xaml::Controls::ListView^>(__target);
            (safe_cast<::Windows::UI::Xaml::Controls::ListView^>(this->listOfWaypoints))->SelectionChanged += ref new ::Windows::UI::Xaml::Controls::SelectionChangedEventHandler(this, (void (::RoutesOverBingMapsApp::MainPage::*)
                (::Platform::Object^, ::Windows::UI::Xaml::Controls::SelectionChangedEventArgs^))&MainPage::OnSelectWaypoint);
        }
        break;
    case 8:
        {
            this->clearWaypointsButton = safe_cast<::Windows::UI::Xaml::Controls::AppBarButton^>(__target);
            (safe_cast<::Windows::UI::Xaml::Controls::AppBarButton^>(this->clearWaypointsButton))->Click += ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::RoutesOverBingMapsApp::MainPage::*)
                (::Platform::Object^, ::Windows::UI::Xaml::RoutedEventArgs^))&MainPage::OnClickClearButton);
        }
        break;
    case 9:
        {
            this->removeWaypointButton = safe_cast<::Windows::UI::Xaml::Controls::AppBarButton^>(__target);
            (safe_cast<::Windows::UI::Xaml::Controls::AppBarButton^>(this->removeWaypointButton))->Click += ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::RoutesOverBingMapsApp::MainPage::*)
                (::Platform::Object^, ::Windows::UI::Xaml::RoutedEventArgs^))&MainPage::OnClickRemoveButton);
        }
        break;
    case 10:
        {
            this->addWaypointButton = safe_cast<::Windows::UI::Xaml::Controls::AppBarButton^>(__target);
            (safe_cast<::Windows::UI::Xaml::Controls::AppBarButton^>(this->addWaypointButton))->Click += ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::RoutesOverBingMapsApp::MainPage::*)
                (::Platform::Object^, ::Windows::UI::Xaml::RoutedEventArgs^))&MainPage::OnClickAddWaypointButton);
        }
        break;
    case 11:
        {
            this->findRouteButton = safe_cast<::Windows::UI::Xaml::Controls::AppBarButton^>(__target);
            (safe_cast<::Windows::UI::Xaml::Controls::AppBarButton^>(this->findRouteButton))->Click += ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::RoutesOverBingMapsApp::MainPage::*)
                (::Platform::Object^, ::Windows::UI::Xaml::RoutedEventArgs^))&MainPage::OnClickFindRouteButton);
        }
        break;
    case 14:
        {
            this->transportTextBlock = safe_cast<::Windows::UI::Xaml::Controls::TextBlock^>(__target);
        }
        break;
    case 15:
        {
            this->useCarButton = safe_cast<::Windows::UI::Xaml::Controls::AppBarToggleButton^>(__target);
            (safe_cast<::Windows::UI::Xaml::Controls::AppBarToggleButton^>(this->useCarButton))->Checked += ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::RoutesOverBingMapsApp::MainPage::*)
                (::Platform::Object^, ::Windows::UI::Xaml::RoutedEventArgs^))&MainPage::OnCheckUseCarButton);
        }
        break;
    case 16:
        {
            this->useTransitButton = safe_cast<::Windows::UI::Xaml::Controls::AppBarToggleButton^>(__target);
            (safe_cast<::Windows::UI::Xaml::Controls::AppBarToggleButton^>(this->useTransitButton))->Checked += ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::RoutesOverBingMapsApp::MainPage::*)
                (::Platform::Object^, ::Windows::UI::Xaml::RoutedEventArgs^))&MainPage::OnCheckUseTransitButton);
        }
        break;
    case 17:
        {
            this->useWalkingButton = safe_cast<::Windows::UI::Xaml::Controls::AppBarToggleButton^>(__target);
            (safe_cast<::Windows::UI::Xaml::Controls::AppBarToggleButton^>(this->useWalkingButton))->Checked += ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::RoutesOverBingMapsApp::MainPage::*)
                (::Platform::Object^, ::Windows::UI::Xaml::RoutedEventArgs^))&MainPage::OnCheckUseWalkingButton);
        }
        break;
    }
    _contentLoaded = true;
}

::Windows::UI::Xaml::Markup::IComponentConnector^ ::RoutesOverBingMapsApp::MainPage::GetBindingConnector(int __connectionId, ::Platform::Object^ __target)
{
    ::XamlBindingInfo::XamlBindings^ bindings = nullptr;
    switch (__connectionId)
    {
        case 1:
            {
                ::Windows::UI::Xaml::Controls::Page^ element1 = safe_cast<::Windows::UI::Xaml::Controls::Page^>(__target);
                MainPage_obj1_Bindings* objBindings = new MainPage_obj1_Bindings();
                objBindings->SetDataRoot(this);
                bindings = ref new ::XamlBindingInfo::XamlBindings(objBindings);
                this->Bindings = bindings;
                element1->Loading += ref new ::Windows::Foundation::TypedEventHandler<::Windows::UI::Xaml::FrameworkElement^, ::Platform::Object^>(bindings, &::XamlBindingInfo::XamlBindings::Loading);
            }
            break;
        case 12:
            {
                ::Windows::UI::Xaml::Controls::StackPanel^ element12 = safe_cast<::Windows::UI::Xaml::Controls::StackPanel^>(__target);
                MainPage_obj12_Bindings* objBindings = new MainPage_obj12_Bindings();
                objBindings->SetDataRoot(element12->DataContext);
                bindings = ref new ::XamlBindingInfo::XamlBindings(objBindings);
                bindings->SubscribeForDataContextChanged(element12);
                ::Windows::UI::Xaml::DataTemplate::SetExtensionInstance(element12, bindings);
            }
            break;
    }
    return bindings;
}

#pragma warning(pop)


