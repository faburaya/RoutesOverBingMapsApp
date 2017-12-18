#pragma once

#include <string>
#include <utility>

namespace RoutesOverBingMapsApp
{
    using namespace Windows::UI::Xaml;
    using namespace Windows::Services::Maps;
    using namespace Windows::Devices::Geolocation;

    using Windows::Foundation::Collections::IObservableVector;


    /// <summary>
    /// Internal type that holds basic information
    /// regarding a geographic location.
    /// </summary>
    struct GeoLocation
    {
        Platform::String ^address;
        BasicGeoposition coordinates;

        size_t GetHashCode()
        {
            size_t result(17);
            result = 23 * result + std::hash<std::wstring>()(std::wstring(address->Data()));
            result = 23 * result + std::hash<double>()(coordinates.Latitude);
            result = 23 * result + std::hash<double>()(coordinates.Longitude);
            return result;
        }
    };

    /// <summary>
    /// Holds info regarding a waypoint in the app view model.
    /// This is also the view model itself for <see cref="MyWaypointControl" />.
    /// </summary>
    public ref class Waypoint sealed
    {
    private:

        int m_order;

        size_t m_lastVerifGLHashCode;

        GeoLocation m_location;

        Geopoint ^m_geocodeQueryHint;

        /// <summary>
        /// Calculates the hash code for the current geographic location data.
        /// </summary>
        /// <returns>
        /// A numeric hash code for the geographic location data.
        /// </returns>
        size_t GetGeoLocHashCode()
        {
            size_t result(13);
            result = 37 * result + m_location.GetHashCode();
            result = 37 * result + m_order;
            return result;
        }

    internal:

        /// <summary>
        /// Gets the geographic location.
        /// </summary>
        /// <returns>
        /// The address and coordinates for the geographic location.
        /// </returns>
        GeoLocation &GetLocation() { return m_location; }

    public:

        Waypoint(int index, Geopoint ^geoCodeQueryHint);

        property Waypoint ^Self
        {
            Waypoint ^get() { return this; }
        }

        /// <summary>
        /// Gets or sets the order of this waypoint in the
        /// list of all waypoints to visit in the route.
        /// </summary>
        /// <value>
        /// The index (1 based).
        /// </value>
        property int Order
        {
            int get() { return m_order; }
            void set(int value) { m_order = value; }
        }

        /// <summary>
        /// Gets the hint for geocoding query.
        /// </summary>
        /// <value>
        /// The geocode query hint.
        /// </value>
        property Geopoint ^GeocodeQueryHint
        {
            Geopoint ^get() { return m_geocodeQueryHint; }
        }

        /// <summary>
        /// Tells whether the current location has been verified by geocoding.
        /// </summary>
        /// <value>
        ///   <c>true</c> if current location has been verified; otherwise, <c>false</c>.
        /// </value>
        property bool IsVerified
        {
            bool get() { return GetGeoLocHashCode() == m_lastVerifGLHashCode; }
        }

        /// <summary>
        /// Marks the current state of this instance
        /// (location data) as verified by geocoding.
        /// </summary>
        void MarkVerified()
        {
            m_lastVerifGLHashCode = GetGeoLocHashCode();
        }
    };


    /// <summary>
    /// Enumerates the options for web services that provide routes.
    /// </summary>
    public enum class RouteService
    {
        Microsoft
        ,GoogleMaps
        ,Tomtom
    };


    /// <summary>
    /// Performs conversion between real number and string.
    /// </summary>
    /// <seealso cref="Data::IValueConverter" />
    public ref class ConverterString2Double sealed : public Data::IValueConverter
    {
    public:

        virtual Platform::Object ^Convert(Platform::Object ^value,
                                          Interop::TypeName targetType,
                                          Platform::Object ^parameter,
                                          Platform::String ^language);

        virtual Platform::Object ^ConvertBack(Platform::Object ^value,
                                              Interop::TypeName targetType,
                                              Platform::Object ^parameter,
                                              Platform::String ^language);
    };


    /// <summary>
    /// The application view model.
    /// </summary>
    public ref class AppViewModel sealed
    {
    private:

        IObservableVector<Waypoint ^> ^m_waypoints;

        RouteService m_routeService;
        
    public:

        AppViewModel();

        /// <summary>
        /// Gets the list of waypoints (set by the user) for the route.
        /// </summary>
        /// <value>
        /// The list of <see cref="Waypoint"/> objects.
        /// </value>
        property IObservableVector<Waypoint ^> ^Waypoints
        {
            IObservableVector<Waypoint ^> ^get() { return m_waypoints; }
        }

        /// <summary>
        /// Gets or sets the route service.
        /// </summary>
        /// <value>
        /// The code for route service.
        /// </value>
        property RouteService Service
        {
            RouteService get() { return m_routeService; }

            void set(RouteService value) { m_routeService = value; }
        }
    };

}// end of namespace RoutesOverBingMapsApp
