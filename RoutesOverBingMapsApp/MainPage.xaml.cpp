//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "TheMap.h"

using namespace RoutesOverBingMapsApp;

using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

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

    useCarButton->IsChecked = true;
    useTransitButton->IsChecked = false;
    useWalkingButton->IsChecked = false;

    TheMap::Initialize(mapControl);
}


/// <summary>
/// Called when the (toggle)button to choose car as transport is checked.
/// </summary>
/// <param name="sender">The sender (control).</param>
/// <param name="evArgs">The event arguments.</param>
void MainPage::OnCheckUseCarButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs)
{
    ViewModel->Transport = TransportOption::Car;
    transportTextBlock->Text = Platform::StringReference(L"By car");
    useTransitButton->IsChecked = false;
    useWalkingButton->IsChecked = false;
}


/// <summary>
/// Called when the (toggle)button to choose transit as transport is checked.
/// </summary>
/// <param name="sender">The sender (control).</param>
/// <param name="evArgs">The event arguments.</param>
void MainPage::OnCheckUseTransitButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs)
{
    ViewModel->Transport = TransportOption::Transit;
    transportTextBlock->Text = Platform::StringReference(L"Transit");
    useCarButton->IsChecked = false;
    useWalkingButton->IsChecked = false;
}


/// <summary>
/// Called when the (toggle)button to choose waling as transport is checked.
/// </summary>
/// <param name="sender">The sender (control).</param>
/// <param name="evArgs">The event arguments.</param>
void MainPage::OnCheckUseWalkingButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs)
{
    ViewModel->Transport = TransportOption::Walking;
    transportTextBlock->Text = Platform::StringReference(L"Walking");
    useCarButton->IsChecked = false;
    useTransitButton->IsChecked = false;
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
}
