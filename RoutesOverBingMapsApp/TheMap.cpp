#include "pch.h"
#include "TheMap.h"
#include <cstring>
#include <algorithm>
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

            static auto typePolylineFName = (ref new MapPolyline)->GetType()->FullName;

            // got the polyline? skip it
            if (element->GetType()->FullName->Equals(typePolylineFName))
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

        RecreatePolyline();
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
            RecreatePolyline();
        }
    }


    /// <summary>
    /// Clear all elements from the app map.
    /// </summary>
    void TheMap::ClearWaypoints()
    {
        m_mapControl->MapElements->Clear();
    }


    /// <summary>
    /// Recreates the polyline for the route.
    /// </summary>
    void TheMap::RecreatePolyline()
    {
        /* This will accumulate key data to define the geo-bounding-box
           that encompasses the whole route in the map view */
        struct {
            double loLatitude, hiLatitude;
            double loLongitude, hiLongitude;
        } geoBox;

        geoBox.loLatitude = geoBox.loLongitude = +180.0;
        geoBox.hiLatitude = geoBox.hiLongitude = -180.0;

        bool plNotFound(true);

        static auto typePolylineFName = (ref new MapPolyline)->GetType()->FullName;

        auto positions = ref new Platform::Collections::Vector<BasicGeoposition, BGeoPosComparator>();

        for (uint32 idx = 0; idx < m_mapControl->MapElements->Size; ++idx)
        {
            MapElement ^element = m_mapControl->MapElements->GetAt(idx);

            // found polyline?
            if (plNotFound && element->GetType()->FullName->Equals(typePolylineFName))
            {
                m_mapControl->MapElements->RemoveAt(idx);
                plNotFound = false;
                continue;
            }

            MapIcon ^pin = safe_cast<MapIcon ^> (m_mapControl->MapElements->GetAt(idx));

            if (pin->Location->Position.Latitude < geoBox.loLatitude)
                geoBox.loLatitude = pin->Location->Position.Latitude;

            if (pin->Location->Position.Latitude > geoBox.hiLatitude)
                geoBox.hiLatitude = pin->Location->Position.Latitude;

            if (pin->Location->Position.Longitude < geoBox.loLongitude)
                geoBox.loLongitude = pin->Location->Position.Longitude;

            if (pin->Location->Position.Longitude > geoBox.hiLongitude)
                geoBox.hiLongitude = pin->Location->Position.Longitude;

            positions->Append(pin->Location->Position);
        }

        // no remaining pin's?
        if (positions->Size == 0)
            return;

        /* Are the latitude values of the box distant enough
        to be seen closer in the oposite face of the earth? */
        if (geoBox.hiLatitude - geoBox.loLatitude > 180.0)
        {
            // switch up and down:
            auto temp = geoBox.hiLatitude;
            geoBox.hiLatitude = geoBox.loLatitude;
            geoBox.loLatitude = temp;
        }

        /* Are the longitude values of the box distant enough
        to be seen closer in the oposite face of the earth? */
        if (geoBox.hiLongitude - geoBox.loLongitude > 180.0)
        {
            // switch left and right:
            auto temp = geoBox.hiLongitude;
            geoBox.hiLongitude = geoBox.loLongitude;
            geoBox.loLongitude = temp;
        }

        double lgtDistDegsFromBorder = std::max(0.05 * (geoBox.hiLongitude - geoBox.loLongitude), 0.01);
        double lttDistDegsFromBorder = std::max(0.05 * (geoBox.hiLatitude - geoBox.loLatitude), 0.01);

        BasicGeoposition nwCorner;
        nwCorner.Altitude = 0.0;
        nwCorner.Latitude = geoBox.hiLatitude + lttDistDegsFromBorder;
        nwCorner.Longitude = geoBox.loLongitude - lgtDistDegsFromBorder;

        BasicGeoposition seCorner;
        seCorner.Altitude = 0.0;
        seCorner.Latitude = geoBox.loLatitude - lttDistDegsFromBorder;
        seCorner.Longitude = geoBox.hiLongitude + lgtDistDegsFromBorder;

        concurrency::create_task(
            m_mapControl->TrySetViewBoundsAsync(ref new GeoboundingBox(nwCorner, seCorner), nullptr, MapAnimationKind::Bow)
        )
        .then([this, positions](bool success)
        {
            if (positions->Size < 2)
                return;

            auto polyline = ref new MapPolyline();
            polyline->Path = ref new Geopath(positions);
            polyline->StrokeColor = Windows::UI::Colors::Black;
            polyline->StrokeThickness = 2;
            polyline->StrokeDashed = true;

            m_mapControl->MapElements->Append(polyline);
        });
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
