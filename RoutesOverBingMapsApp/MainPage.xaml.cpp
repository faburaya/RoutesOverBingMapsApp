//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "Utils.h"
#include "TheMap.h"
#include "RouteHelper.h"
#include <3FD\runtime.h>
#include <3FD\utils_winrt.h>


using namespace _3fd;
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
    useAllServicesButton->IsChecked = false;
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
    useAllServicesButton->IsChecked = false;
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
    useAllServicesButton->IsChecked = false;
}


/// <summary>
/// Called when the (toggle)button to choose all route services is checked.
/// </summary>
/// <param name="sender">The sender (control).</param>
/// <param name="evArgs">The event arguments.</param>
void MainPage::OnCheckUseAllServicesButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs)
{
    ViewModel->Service = RouteService::All;
    serviceTextBlock->Text = Platform::StringReference(L"All of them");
    useMicrosoftButton->IsChecked = false;
    useGoogleButton->IsChecked = false;
    useTomtomButton->IsChecked = false;
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
void MainPage::OnClickClearWaypointsButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs)
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
/// Called when the button to clear routes is clicked.
/// </summary>
/// <param name="sender">The sender (control).</param>
/// <param name="evArgs">The event arguments.</param>
void MainPage::OnClickClearRoutesButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs)
{
    TheMap::GetInstance().ClearRoutesPolylines();

    mapControl->Routes->Clear();
    ViewModel->Routes->Clear();

    useMicrosoftButton->IsEnabled = true;
    useGoogleButton->IsEnabled = true;
    useTomtomButton->IsEnabled = true;
    useAllServicesButton->IsEnabled = true;

    listOfRoutes->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    listOfWaypoints->Visibility = Windows::UI::Xaml::Visibility::Visible;
    clearRoutesButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    clearWaypointsButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
    removeWaypointButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
    addWaypointButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
    findRouteButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
}


// default parameters for exception notification and logging
static const _3fd::utils::UwpXaml::ExNotifAndLogParams exHndParams
{
    Platform::StringReference(L"Application error!\n"),
    Platform::StringReference(L"Cancel"),
    _3fd::core::Logger::PRIO_ERROR
};


/// <summary>
/// Called when the button to request routes (from web services) is clicked.
/// </summary>
/// <param name="sender">The sender ("request route" button).</param>
/// <param name="evArgs">The event arguments.</param>
void MainPage::OnClickFindRouteButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs)
{
    core::FrameworkInstance fffd("RoutesOverBingMapsApp");

    CALL_STACK_TRACE;

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

    // Find routes with the chosen service:

    waitingRing->IsActive = true;
    waitingRing->Visibility = Windows::UI::Xaml::Visibility::Visible;

    std::vector<task<GeoboundingBox ^>> tasks;
    tasks.reserve(3);

    std::shared_ptr<RouteColorPicker> colorPicker(new RouteColorPicker());
    
    ////////////////////
    // Microsoft:
    
    if (ViewModel->Service == RouteService::Microsoft || ViewModel->Service == RouteService::All)
    {
        auto thisTask = GetRoutesFromMicrosoftAsync(
            ViewModel->Waypoints->GetView(),
            MapRouteOptimization::Time,
            static_cast<uint8> (RouteRestriction::AvoidDirt | RouteRestriction::AvoidFerries)
        )
        .then([this, colorPicker](IVector<MapRoute ^> ^routes)
        {
            for (auto route : routes)
            {
                auto routeInfo = ref new RouteInfo(route, *colorPicker);
                auto routeView = ref new MapRouteView(route);
                routeView->RouteColor = routeInfo->LineColor;
                mapControl->Routes->Append(routeView);
                ViewModel->Routes->Append(routeInfo);
            }

            return MergeViewsBoundaries<MapRoute ^>(
                begin(routes),
                end(routes),
                [](MapRoute ^entry) { return entry->BoundingBox; }
            );
        });

        tasks.push_back(thisTask);
    }

    ////////////////////
    // Google Maps:
    
    if (ViewModel->Service == RouteService::GoogleMaps || ViewModel->Service == RouteService::All)
    {
        auto thisTask = GetRoutesFromGoogleAsync(
            ViewModel->Waypoints->GetView(),
            nullptr,
            static_cast<uint8> (RouteRestriction::AvoidFerries)
        )
        .then([this, colorPicker](RoutesFromWebApi results)
        {
            std::vector<GeoboundingBox ^> bounds;
            bounds.reserve(results->size());

            for (auto &route : *results)
            {
                std::vector<BasicGeoposition> path;
                route->GeneratePath(path);

                auto routeInfo = ref new RouteInfo(RouteService::GoogleMaps,
                                                   route->GetMainInfo(),
                                                   route->GetMoreInfo(),
                                                   *colorPicker);

                TheMap::GetInstance().DisplayRouteAsPolyline(std::move(path), routeInfo->LineColor);

                bounds.push_back(route->GetBounds());
                ViewModel->Routes->Append(routeInfo);
            }

            return MergeViewsBoundaries(bounds.begin(), bounds.end());

        }, task_continuation_context::use_current());

        tasks.push_back(thisTask);
    }

    ////////////////////
    // TomTom:

    if (ViewModel->Service == RouteService::Tomtom || ViewModel->Service == RouteService::All)
    {

    }

    // wait for all services to respond:
    when_all(tasks.begin(), tasks.end()).then([this](task<std::vector<GeoboundingBox ^>> priorTasks)
    {
        std::vector<GeoboundingBox ^> bounds;

        if (utils::UwpXaml::GetTaskRetAndHndEx(priorTasks, bounds, exHndParams) == STATUS_OKAY)
        {
            Thickness borders{ 80, 80, 80, 80 };

            GeoboundingBox ^mergedBounds = MergeViewsBoundaries(bounds.begin(), bounds.end());
            
            mapControl->TrySetViewBoundsAsync(mergedBounds, borders, MapAnimationKind::Linear);

            useMicrosoftButton->IsEnabled = false;
            useGoogleButton->IsEnabled = false;
            useTomtomButton->IsEnabled = false;
            useAllServicesButton->IsEnabled = false;

            listOfRoutes->Visibility = Windows::UI::Xaml::Visibility::Visible;
            listOfWaypoints->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
            clearRoutesButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
            clearWaypointsButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
            removeWaypointButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
            addWaypointButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
            findRouteButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        }

        waitingRing->IsActive = false;
        waitingRing->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    
    }, task_continuation_context::use_current());
}
