﻿//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "TheMap.h"
#include <3FD\utils_winrt.h>


using namespace RoutesOverBingMapsApp;

using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

/// <summary>
/// Constructor of the main page.
/// </summary>
MainPage::MainPage()
    : m_viewModel(ref new AppViewModel())
    , m_nextWayptOrderNum(1)
{
	InitializeComponent();

    MapService::ServiceToken =
        Platform::StringReference(L"UBu2mHom1aG4EwNAhp6k~Wov5AskUExUDvuaCIrSjng~ApMoRijnwq9SsGFkuRe3T7zKxK9l9OKQtFC-AKU_FSYV0cLYtcJdyMzv-ptVz5lP");

    useMicrosoftButton->IsChecked = true;
    useGoogleButton->IsChecked = false;
    useTomtomButton->IsChecked = false;

    TheMap::Initialize(mapControl);
}


// default parameters for exception notification and logging
static const _3fd::utils::UwpXaml::ExNotifAndLogParams exHndParams
{
    Platform::StringReference(L"Application error!\n"),
    Platform::StringReference(L"Cancel"),
    _3fd::core::Logger::PRIO_ERROR
};


/// <summary>
/// Called when the (toggle)button to choose Bing Maps service is checked.
/// </summary>
/// <param name="sender">The sender (control).</param>
/// <param name="evArgs">The event arguments.</param>
void MainPage::OnCheckUseMicrosoftButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs)
{
    ViewModel->Service = RouteService::Microsoft;
    serviceTextBlock->Text = Platform::StringReference(L"Microsoft");
    useGoogleButton->IsChecked = false;
    useTomtomButton->IsChecked = false;
}


/// <summary>
/// Called when the (toggle)button to choose Google Maps API is checked.
/// </summary>
/// <param name="sender">The sender (control).</param>
/// <param name="evArgs">The event arguments.</param>
void MainPage::OnCheckUseGoogleButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs)
{
    ViewModel->Service = RouteService::GoogleMaps;
    serviceTextBlock->Text = Platform::StringReference(L"Google Maps");
    useMicrosoftButton->IsChecked = false;
    useTomtomButton->IsChecked = false;
}


/// <summary>
/// Called when the (toggle)button to choose Tomtom API is checked.
/// </summary>
/// <param name="sender">The sender (control).</param>
/// <param name="evArgs">The event arguments.</param>
void MainPage::OnCheckUseTomtomButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs)
{
    ViewModel->Service = RouteService::Tomtom;
    serviceTextBlock->Text = Platform::StringReference(L"Tomtom");
    useMicrosoftButton->IsChecked = false;
    useGoogleButton->IsChecked = false;
}


/// <summary>
/// Called when a waypoint is selected in the list.
/// </summary>
/// <param name="sender">The sender (control).</param>
/// <param name="evArgs">The event arguments.</param>
void MainPage::OnSelectWaypoint(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs)
{
    auto selArgs = safe_cast<Windows::UI::Xaml::Controls::SelectionChangedEventArgs ^> (evArgs);

    _ASSERTE(selArgs->AddedItems->Size <= 1);

    removeWaypointButton->IsEnabled = (selArgs->AddedItems->Size == 1);
}


/// <summary>
/// Called when the button to add a waypoint to the path is clicked.
/// </summary>
/// <param name="sender">The sender (control).</param>
/// <param name="evArgs">The event arguments.</param>
void MainPage::OnClickAddWaypointButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs)
{
    auto waypoint =
        ref new Waypoint(m_nextWayptOrderNum++,
                         mapControl->Center);

    // add waypoint to the list
    ViewModel->Waypoints->Append(waypoint);

    clearWaypointsButton->IsEnabled = true;
    findRouteButton->IsEnabled = (ViewModel->Waypoints->Size > 1);
}


/// <summary>
/// Called when the button to remove a waypoint from the path is clicked.
/// </summary>
/// <param name="sender">The sender (control).</param>
/// <param name="evArgs">The event arguments.</param>
void MainPage::OnClickRemoveButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs)
{
    int selectedWaypointIndex = listOfWaypoints->SelectedIndex;

    if (selectedWaypointIndex > -1)
    {
        // remove from the map:
        TheMap::GetInstance().RemoveWaypoint(ViewModel->Waypoints->GetAt(selectedWaypointIndex)->Order);

        // remove from the list:
        ViewModel->Waypoints->RemoveAt(selectedWaypointIndex);
    }

    clearWaypointsButton->IsEnabled = (ViewModel->Waypoints->Size > 0);
    findRouteButton->IsEnabled = (ViewModel->Waypoints->Size > 1);
}


/// <summary>
/// Called when the button to clear all waypoints is clicked.
/// </summary>
/// <param name="sender">The sender (control).</param>
/// <param name="evArgs">The event arguments.</param>
void MainPage::OnClickClearButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs)
{
    // clear list:
    ViewModel->Waypoints->Clear();

    // clear map:
    TheMap::GetInstance().ClearWaypoints();

    m_nextWayptOrderNum = 1;

    clearWaypointsButton->IsEnabled = false;
    findRouteButton->IsEnabled = false;
}


/// <summary>
/// Called when the button to request routes (from web services) is clicked.
/// </summary>
/// <param name="sender">The sender ("request route" button).</param>
/// <param name="evArgs">The event arguments.</param>
void MainPage::OnClickFindRouteButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs)
{
    uint32 idx(0);

    // iterate through the list of waypoints and get rid of the unverified ones:
    while (idx < ViewModel->Waypoints->Size)
    {
        Waypoint ^waypoint = ViewModel->Waypoints->GetAt(idx);

        if (waypoint->IsVerified)
            ++idx;
        else
        {
            // remove from the map:
            TheMap::GetInstance().RemoveWaypoint(waypoint->Order);

            // remove from the list:
            ViewModel->Waypoints->RemoveAt(idx);
        }
    }

    // TO DO
}
