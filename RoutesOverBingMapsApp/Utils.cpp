#include "pch.h"
#include "Utils.h"
#include <set>
#include <sstream>
#include <iomanip>
#include <cinttypes>


namespace RoutesOverBingMapsApp
{
    /// <summary>
    /// Rounds a duration given in seconds and creates a textual representation.
    /// </summary>
    /// <param name="seconds">The duration in seconds.</param>
    /// <returns>Formatted time span text (like "5h 30 min").</returns>
    std::wstring ToTimeSpanText(uint32_t seconds)
    {
        if (seconds < 60)
            return L"1 min";

        std::wostringstream woss;

        int hours = seconds / 3600;
        int minutes = (seconds - 3600 * hours) / 60;

        if (hours != 0)
        {
            woss << hours << L" h";

            if (minutes != 0)
                woss << L' ';
        }

        int remainingSecs = seconds - 3600 * hours - 60 * seconds;

        if (remainingSecs >= 30)
            ++minutes;

        if (minutes != 0)
            woss << minutes << " min";

        return woss.str();
    }


    /// <summary>
    /// Creates a textual representation for distance given in meters.
    /// </summary>
    /// <param name="meters">The distance in meters.</param>
    /// <returns>Formatted distance text (like "1.54 km").</returns>
    std::wstring ToDistanceText(uint32_t meters)
    {
        std::wostringstream woss;

        if (meters > 1000)
            woss << std::setprecision(3) << (meters / 1000) << L" km";
        else
            woss << meters << " meters";

        return woss.str();
    }


    /// <summary>
    /// Calculates the boundaries of a view that covers all the
    /// geographic positions provided in the given list.
    /// </summary>
    /// <remarks>
    /// This is a more reliable approach for getting the actual boundaries of a route. For
    /// example, Google Maps Directions API in its JSON response provides bondaries that do
    /// not take into account "detour's" of the route, so we cannot use such a result because
    /// then we would end up with a view box that does not enclose the route in its entirety.
    /// </remarks>
    /// <param name="getPositions">
    /// A list of <see cref="Windows::Devices::Geolocation::BasicGeoposition"/> objects
    /// for the POI's in the map.
    /// </param>
    /// <returns>
    /// A <see cref="Windows::Devices::Geolocation::GeoboundingBox"/> object for
    /// a view that covers all geographic positions provided in the list range.
    /// </returns>
    GeoboundingBox ^CalculateViewBoundaries(const std::vector<BasicGeoposition> &geoPositions)
    {
        _ASSERTE(!geoPositions.empty()); // cannot be empty

        std::set<double> latitudes;
        std::set<double> longitudes;

        // Store all latitude and longitude values separately and in order:
        for (auto &position : geoPositions)
        {
            latitudes.insert(position.Latitude);
            longitudes.insert(position.Longitude);
        }

        // stores the geographic boundaries of a view
        struct {
            double west, east;
            double south, north;
        } bounds;

        double interval;
        double largestInterval(0.0);

        auto prevIter = latitudes.begin();

        // scan the latitudes in order, looking for the largest empty interval:
        for (auto iter = ++latitudes.begin(); iter != latitudes.end(); ++iter)
        {
            interval = *iter - *prevIter;

            if (interval > largestInterval)
            {
                largestInterval = interval;
                bounds.south = *prevIter;
                bounds.north = *iter;
            }

            prevIter = iter;
        }

        /* additionally, check whether the lowest and the highest
        latitudes look closer on the other side of the earth: */

        interval = (*latitudes.begin() + 90.0) + (90.0 - *latitudes.rbegin());

        if (interval > largestInterval)
        {
            bounds.south = *latitudes.rbegin();
            bounds.north = *latitudes.begin();
        }

        largestInterval = 0.0;

        prevIter = longitudes.begin();

        // scan the longitudes in order, looking for the largest empty interval:
        for (auto iter = ++longitudes.begin(); iter != longitudes.end(); ++iter)
        {
            interval = *iter - *prevIter;

            if (interval > largestInterval)
            {
                largestInterval = interval;
                bounds.west = *prevIter;
                bounds.east = *iter;
            }

            prevIter = iter;
        }

        /* additionally, check whether the lowest and the highest
           longitudes look closer on the other side of the earth: */

        interval = (*longitudes.begin() + 180.0) + (180.0 - *longitudes.rbegin());

        if (interval > largestInterval)
        {
            bounds.west = *longitudes.rbegin();
            bounds.east = *longitudes.begin();
        }

        /* at this point we have the largest empty view of the earth, so
           if we take the opposite view (from the other side of the earth),
           we have the view that covers all the scanned positions: */

        BasicGeoposition northwestCorner;
        northwestCorner.Altitude = 0.0;
        northwestCorner.Latitude = bounds.south; // swap south & north
        northwestCorner.Longitude = bounds.east; // swap west & east

        BasicGeoposition southeastCorner;
        southeastCorner.Altitude = 0.0;
        southeastCorner.Latitude = bounds.north; // swap south & north
        southeastCorner.Longitude = bounds.west; // swap west & east

        // finally, add some extra to the boundaries, so the content does not touch the border:

        double latDistFromBorder = std::max(0.03 * (bounds.north - bounds.south), 0.01);
        double lngDistFromBorder = std::max(0.03 * (bounds.east - bounds.west), 0.01);

        northwestCorner.Latitude += latDistFromBorder;
        northwestCorner.Longitude -= lngDistFromBorder;
        southeastCorner.Latitude -= latDistFromBorder;
        southeastCorner.Longitude += lngDistFromBorder;

        return ref new GeoboundingBox(northwestCorner, southeastCorner);
    }

}// end of namespace RoutesOverBingMapsApp