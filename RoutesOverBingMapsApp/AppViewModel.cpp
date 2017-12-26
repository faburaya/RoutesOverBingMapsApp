//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "AppViewModel.h"
#include "Utils.h"
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <array>
#include <list>

using namespace RoutesOverBingMapsApp;


///////////////////////
// Waypoint Class
///////////////////////

/// <summary>
/// Initializes a new instance of the <see cref="Waypoint" /> class.
/// </summary>
/// <param name="index">The waypoint index.</param>
/// <param name="geoCodeQueryHint">A hint for the geocoding query.</param>
Waypoint::Waypoint(int index, Geopoint ^geoCodeQueryHint)
    : m_order(index)
    , m_lastVerifGLHashCode(0)
    , m_geocodeQueryHint(geoCodeQueryHint)
{
    m_location.address = Platform::StringReference(L"");
    m_location.coordinates = BasicGeoposition{ 0.0, 0.0, 0.0 };
}


////////////////////////////
// RouteColorPicker Class
////////////////////////////


/// <summary>
/// Enumerates all the possibilities for route color.
/// </summary>
std::array<Windows::UI::Color, 14> RouteColorPicker::routeColorOptions =
{
    Windows::UI::Colors::Beige,
    Windows::UI::Colors::Blue,
    Windows::UI::Colors::Brown,
    Windows::UI::Colors::Gold,
    Windows::UI::Colors::DarkGray,
    Windows::UI::Colors::Green,
    Windows::UI::Colors::Magenta,
    Windows::UI::Colors::Orange,
    Windows::UI::Colors::Purple,
    Windows::UI::Colors::Red,
    Windows::UI::Colors::SeaGreen,
    Windows::UI::Colors::Violet,
    Windows::UI::Colors::White,
    Windows::UI::Colors::YellowGreen
};


/// <summary>
/// Initializes a new instance of the <see cref="RouteColorPicker"/> class.
/// </summary>
RouteColorPicker::RouteColorPicker()
    : m_remainingOptions(routeColorOptions.begin(), routeColorOptions.end())
{
    srand(time(nullptr));
}


/// <summary>
/// Gets a distinct color (if there are remaining options).
/// </summary>
/// <returns>An option of color.</returns>
Windows::UI::Color RouteColorPicker::GetDistinctColor()
{
    if (m_remainingOptions.empty())
        return Windows::UI::Colors::Red;

    auto idx = static_cast<uint32_t> (rand() % m_remainingOptions.size());
    auto iter = std::next(m_remainingOptions.begin(), idx);
    Windows::UI::Color color = *iter;
    m_remainingOptions.erase(iter);
    return color;
}


///////////////////////
// RouteInfo Class
///////////////////////


/// <summary>
/// Initializes a new instance of the <see cref="RouteInfo"/> class.
/// </summary>
/// <param name="route">
/// The <see cref="Windows::Services::Maps::MapRoute"/> object returned by WinRT Maps Services.
/// </param>
RouteInfo::RouteInfo(MapRoute ^route, RouteColorPicker &colorPicker)
{
    m_bgBrush = ref new Media::SolidColorBrush(colorPicker.GetDistinctColor());
    m_bgBrush->Opacity = 0.80;

    std::wostringstream woss;

    woss << ToTimeSpanText(static_cast<uint32_t> (route->EstimatedDuration.Duration * 100 * 1e-9))
         << L" (" << ToDistanceText(static_cast<uint32_t> (route->LengthInMeters + 0.5)) << L')';

    m_mainInfo = ref new Platform::String(woss.str().c_str());

    woss.str(L"");

    if (route->IsTrafficBased)
        woss << L"Time estimative is based on traffic data";
    else
        woss << L"Time was estimated WITHOUT traffic!";

    auto violatedRestrictions = static_cast<uint32> (route->ViolatedRestrictions);

    if ((violatedRestrictions & (uint32)MapRouteRestrictions::DirtRoads) != 0)
        woss << L"\nHas dirty roads!";

    if ((violatedRestrictions & (uint32)MapRouteRestrictions::Ferries) != 0)
        woss << L"\nHas ferries!";

    if ((violatedRestrictions & (uint32)MapRouteRestrictions::TollRoads) != 0)
        woss << L"\nHas tolls!";

    if ((violatedRestrictions & (uint32)MapRouteRestrictions::Highways) != 0)
        woss << L"\nHas highways!";

    if (route->HasBlockedRoads)
        woss << L"\nHas blocked roads!";

    m_moreInfo = ref new Platform::String(woss.str().c_str());
}


/// <summary>
/// Initializes a new instance of the <see cref="RouteInfo"/> class.
/// </summary>
/// <param name="service">The service that calculated the route.</param>
/// <param name="mainInfo">The main information about the route, to be shown in the headline.</param>
/// <param name="moreInfo">More information about the route, to be shown in the details.</param>
RouteInfo::RouteInfo(RouteService service,
                     Platform::String ^mainInfo,
                     Platform::String ^moreInfo,
                     RouteColorPicker &colorPicker)
    : m_service(service)
    , m_mainInfo(mainInfo)
    , m_moreInfo(moreInfo)
{
    m_bgBrush = ref new Media::SolidColorBrush(colorPicker.GetDistinctColor());
    m_bgBrush->Opacity = 0.80;
}


///////////////////////
// AppViewModel Class
///////////////////////


/// <summary>
/// Initializes a new instance of the <see cref="AppViewModel"/> class.
/// </summary>
AppViewModel::AppViewModel()
    : m_waypoints(ref new Platform::Collections::Vector<Waypoint ^>())
    , m_routesInfo(ref new Platform::Collections::Vector<RouteInfo ^>())
    , m_routeService(RouteService::Microsoft)
{
}


////////////////////////////////////
// ConverterString2Double Class
////////////////////////////////////


/// <summary>
/// Converts the string to a real number.
/// </summary>
/// <param name="value">The value to convert (string).</param>
/// <param name="targetType">Target type (<see cref="TransportOption"/>).</param>
/// <param name="parameter">An extra parameter for the conversion (not used here).</param>
/// <param name="language">The language (not used here).</param>
/// <returns>The real number serialized to a string.</returns>
Platform::Object ^ConverterString2Double::Convert(Platform::Object ^value,
                                                  Interop::TypeName targetType,
                                                  Platform::Object ^parameter,
                                                  Platform::String ^language)
{
    static Platform::String ^dummy = Platform::StringReference(L"dummy");
    static Platform::String ^stringTypeName = dummy->GetType()->FullName;

    _ASSERTE((ref new Platform::Type(targetType))->FullName->Equals(stringTypeName));

    wchar_t buffer[16];
    _snwprintf(buffer, sizeof buffer, L"%g", safe_cast<double> (value));

    return ref new Platform::String(buffer);
}


/// <summary>
/// Converts the string to real number.
/// </summary>
/// <param name="value">The value to convert (string).</param>
/// <param name="targetType">Target type (double).</param>
/// <param name="parameter">An extra parameter for the conversion (not used here).</param>
/// <param name="language">The language (not used here).</param>
/// <returns>The real number parsed from text.</returns>
Platform::Object ^ConverterString2Double::ConvertBack(Platform::Object ^value,
                                                      Interop::TypeName targetType,
                                                      Platform::Object ^parameter,
                                                      Platform::String ^language)
{
    static auto doubleTypeName = default::float64(0.0).GetType()->FullName;
    
    _ASSERTE((ref new Platform::Type(targetType))->FullName->Equals(doubleTypeName));

    return wcstod(safe_cast<Platform::String ^> (value)->Data(), nullptr);
}
