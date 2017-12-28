#pragma once

#include <vector>
#include <memory>
#include <cinttypes>
#include <cpprest/json.h>


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

        MapPolyline ^m_itineraryLine;

        Platform::String ^m_typePolylineFName;


        TheMap(MapControl ^mapControl);

        bool FindWaypoint(int wayptOrder, int *whereIdx) const;

        void UpdateItineraryLine();

    public:

        static void Initialize(MapControl ^mapControl);

        static TheMap &GetInstance();

        static void Finalize();

        void DisplayRouteAsPolyline(std::vector<BasicGeoposition> &&path, Windows::UI::Color color);

        void DisplayWaypointLocation(int wayptOrder, BasicGeoposition location);

        void RemoveWaypoint(int wayptOrder);

        void ClearWaypoints();

        void ClearRoutesPolylines();

        void GetCenterPosition(double &latitude, double &longitude);
    };

}// end of namespace RoutesOverBingMapsApp
