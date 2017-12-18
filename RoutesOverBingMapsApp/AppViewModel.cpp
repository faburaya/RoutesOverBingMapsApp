//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "AppViewModel.h"

#include <cstdio>
#include <cstring>

using namespace RoutesOverBingMapsApp;


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


///////////////////////
// AppViewModel Class
///////////////////////

/// <summary>
/// Initializes a new instance of the <see cref="AppViewModel"/> class.
/// </summary>
AppViewModel::AppViewModel()
    : m_waypoints(ref new Platform::Collections::Vector<Waypoint ^>())
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
