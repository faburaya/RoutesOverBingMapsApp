#include "pch.h"
#include "TheMap.h"
#include "Utils.h"
#include <cstring>
#include <algorithm>
#include <memory>
#include <array>

#undef min
#undef max

namespace RoutesOverBingMapsApp
{
    std::unique_ptr<TheMap> TheMap::singleton;


    /// <summary>
    /// Initializes the singleton.
    /// </summary>
    /// <param name="mapControl">The map control.</param>
    void TheMap::Initialize(MapControl ^mapControl)
    {
        _ASSERTE(singleton.get() == nullptr);
        singleton.reset(new TheMap(mapControl));
    }


    /// <summary>
    /// Gets the singleton.
    /// </summary>
    /// <returns>A reference to the singleton</returns>
    TheMap & TheMap::GetInstance()
    {
        _ASSERTE(singleton.get() != nullptr);
        return *singleton;
    }


    /// <summary>
    /// Finalizes the singleton.
    /// </summary>
    void TheMap::Finalize()
    {
        singleton.reset(nullptr);
    }


    /// <summary>
    /// Initializes a new instance of the <see cref="TheMap"/> class.
    /// </summary>
    /// <param name="mapControl">The map control.</param>
    TheMap::TheMap(MapControl ^mapControl)
        : m_mapControl(mapControl)
    {
        m_typePolylineFName = (ref new MapPolyline)->GetType()->FullName;
    }


    /// <summary>
    /// Finds the given waypoint in the list of map elements.
    /// </summary>
    /// <param name="wayptOrder">The waypoint order in the list.</param>
    /// <param name="whereIdx">When not null, receives the index in the
    /// list of map elements where the given waypoint should be placed.</param>
    /// <returns>
    /// Whether the waypoint was found to be removed.
    /// </returns>
    bool TheMap::FindWaypoint(int wayptOrder, int *whereIdx) const
    {
        int vecIdx(wayptOrder);

        if (vecIdx >= m_mapControl->MapElements->Size)
            vecIdx = m_mapControl->MapElements->Size - 1;

        // Iterate the current list o map elements backwards until the waypoint is found:
        while (vecIdx >= 0)
        {
            MapElement ^element = m_mapControl->MapElements->GetAt(vecIdx);

            // got a polyline? skip it
            if (element->GetType()->FullName->Equals(m_typePolylineFName))
            {
                --vecIdx;
                continue;
            }

            MapIcon ^pin = safe_cast<MapIcon ^> (element);
            int pinIndex = static_cast<int> (wcstol(pin->Title->Data(), nullptr, 10));

            if (pinIndex > wayptOrder)
            {
                --vecIdx;
            }
            else if (pinIndex == wayptOrder)
            {
                if (whereIdx != 0)
                    *whereIdx = vecIdx;

                return true;
            }
            else
                break;
        }

        if (whereIdx != nullptr)
            *whereIdx = vecIdx + 1;

        return false;
    }


    /// <summary>
    /// Displays a new route using a polyline.
    /// </summary>
    /// <param name="path">The path of geo locations for the polyline.</param>
    /// <param name="color">The chosen route color.</param>
    void TheMap::DisplayRouteAsPolyline(std::vector<BasicGeoposition> &&path, Windows::UI::Color color)
    {
        auto polyline = ref new MapPolyline();
        polyline->StrokeColor = color;
        polyline->StrokeThickness = 7;
        polyline->ZIndex = 1;

        polyline->Path = ref new Geopath(
            ref new Platform::Collections::Vector<BasicGeoposition, BGeoPosComparator>(std::move(path))
        );

        m_mapControl->MapElements->Append(polyline);
    }


    /// <summary>
    /// Adds or updates in the app map a verified waypoint location.
    /// </summary>
    /// <param name="wayptOrder">The order (1-based) of the given waypoint in the list of waypoints to visit in the route.</param>
    /// <param name="location">The geographic location to display.</param>
    void TheMap::DisplayWaypointLocation(int wayptOrder, BasicGeoposition location)
    {
        int vecIdx;

        // found? remove it
        if (FindWaypoint(wayptOrder, &vecIdx))
        {
            auto pin = safe_cast<MapIcon ^> (m_mapControl->MapElements->GetAt(vecIdx));

            // same waypoint? do nothing
            BGeoPosComparator equal;
            if (equal(pin->Location->Position, location))
                return;

            m_mapControl->MapElements->RemoveAt(vecIdx);
        }

        // title of the waypoint in the map is its index
        std::array<wchar_t, 4> title;
        _snwprintf(title.data(), title.size(), L"%d", wayptOrder);

        auto pin = ref new MapIcon();
        pin->Location = ref new Geopoint(location);
        pin->NormalizedAnchorPoint = Windows::Foundation::Point(0.5, 1.0);
        pin->ZIndex = 0;
        pin->Title = ref new Platform::String(title.data());

        m_mapControl->MapElements->InsertAt(vecIdx, pin);

        UpdateItineraryLine();
    }


    /// <summary>
    /// Removes the waypoint from the list of map elements (if present).
    /// </summary>
    /// <param name="wayptOrder">Order of the waypoint in the list (1-based).</param>
    void TheMap::RemoveWaypoint(int wayptOrder)
    {
        int vecIdx;
        
        if (FindWaypoint(wayptOrder, &vecIdx))
        {
            m_mapControl->MapElements->RemoveAt(vecIdx);
            UpdateItineraryLine();
        }
    }


    /// <summary>
    /// Clear all waypoints from the app map.
    /// </summary>
    void TheMap::ClearWaypoints()
    {
        /* This will also remove polylines for routes and this is fine because
           of the way this app works. That is: by the time you want to remove the
           waypoints, the routes have already been cleared by a previous command.*/
        m_mapControl->MapElements->Clear();
        m_itineraryLine = nullptr;
    }


    /// <summary>
    /// Clear all polylines for routes in the app map.
    /// </summary>
    void TheMap::ClearRoutesPolylines()
    {
        std::vector<MapElement ^> temp;
        temp.reserve(m_mapControl->MapElements->Size);

        for (auto entry : m_mapControl->MapElements)
        {
            auto element = static_cast<MapElement ^> (entry);

            // got a polyline? skip it
            if (element->GetType()->FullName->Equals(m_typePolylineFName))
                continue;

            temp.push_back(element);
        }

        m_mapControl->MapElements->Clear(); // clear all

        // restore the waypoints:
        for (auto entry : temp)
            m_mapControl->MapElements->Append(entry);

        // restore the itinerary line:
        if (m_itineraryLine != nullptr)
            m_mapControl->MapElements->Append(m_itineraryLine);

    }


    /// <summary>
    /// Updates the polyline for the route itinerary (the one that contains all waypoints).
    /// </summary>
    void TheMap::UpdateItineraryLine()
    {
        std::vector<BasicGeoposition> positions;
        positions.reserve(m_mapControl->MapElements->Size);

        for (auto entry : m_mapControl->MapElements)
        {
            auto element = static_cast<MapElement ^> (entry);

            // got a polyline? skip it
            if (element->GetType()->FullName->Equals(m_typePolylineFName))
                continue;

            positions.push_back(safe_cast<MapIcon ^> (element)->Location->Position);
        }

        // no remaining pin's?
        if (positions.empty())
            return;

        m_mapControl->TrySetViewBoundsAsync(CalculateViewBoundaries(positions),
                                            nullptr,
                                            MapAnimationKind::Linear);

        if (positions.size() < 2)
            return;

        if (m_itineraryLine == nullptr)
        {
            // itinerary line has been erased, so re-create it:
            m_itineraryLine = ref new MapPolyline();
            m_itineraryLine->StrokeColor = Windows::UI::Colors::Black;
            m_itineraryLine->StrokeThickness = 2;
            m_itineraryLine->StrokeDashed = true;
            m_itineraryLine->ZIndex = -1;

            // the polyline must have its path set before entering the list of map elements:
            m_itineraryLine->Path = ref new Geopath(
                ref new Platform::Collections::Vector<BasicGeoposition, BGeoPosComparator>(std::move(positions))
            );

            m_mapControl->MapElements->Append(m_itineraryLine);
        }
        else
        {
            // itinerary line already set, so just update its path:
            m_itineraryLine->Path = ref new Geopath(
                ref new Platform::Collections::Vector<BasicGeoposition, BGeoPosComparator>(std::move(positions))
            );
        }
    }


    /// <summary>
    /// Gets the current map center position.
    /// </summary>
    /// <param name="latitude">The latitude.</param>
    /// <param name="longitude">The longitude.</param>
    void TheMap::GetCenterPosition(double &latitude, double &longitude)
    {
        latitude = m_mapControl->Center->Position.Latitude;
        longitude = m_mapControl->Center->Position.Longitude;
    }

}// end of namespace RoutesOverBingMapsApp
