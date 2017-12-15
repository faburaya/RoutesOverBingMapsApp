#pragma once

#include <memory>

namespace RoutesOverBingMapsApp
{
    using namespace Windows::UI::Xaml::Controls::Maps;
    using namespace Windows::Devices::Geolocation;


    /// <summary>
    /// Exposes a basic interface for a child control
    /// to use the map capabilities of the main page.
    /// </summary>
    class TheMap
    {
    private:

        static std::unique_ptr<TheMap> singleton;

        MapControl ^m_mapControl;

        TheMap(MapControl ^mapControl)
            : m_mapControl(mapControl) {}

        bool FindWaypoint(int wayptOrder, int *whereIdx) const;

        void RecreatePolyline();

    public:

        static void Initialize(MapControl ^mapControl);

        static TheMap &GetInstance();

        static void Finalize();

        void DisplayWaypointLocation(int wayptOrder, BasicGeoposition location);

        void RemoveWaypoint(int wayptOrder);

        void ClearWaypoints();

        void GetCenterPosition(double &latitude, double &longitude);
    };


    /// <summary>
    /// Functor for comparison of <see cref="Windows::Devices::Geolocation::BasicGeoposition" /> value types.
    /// </summary>
    /// <remarks>
    /// This is required by <see cref="Platform::Vector"/>, otherwise compilation error C2678 is issued.
    /// </remarks>
    struct BGeoPosComparator
    {
        constexpr bool operator()(const BasicGeoposition &left, const BasicGeoposition &right) const
        {
            return (left.Altitude == right.Altitude
                && left.Latitude == right.Latitude
                && left.Longitude == right.Longitude);
        }
    };

}// end of namespace RoutesOverBingMapsApp
