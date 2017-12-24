#include "pch.h"
#include "Utils.h"
#include <set>


namespace RoutesOverBingMapsApp
{
    /// <summary>
    /// Calculates the boundaries of a view that covers all the
    /// geographic positions provided in the given list.
    /// </summary>
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
                bounds.west = *prevIter;
                bounds.east = *iter;
            }

            prevIter = iter;
        }

        /* additionally, check whether the lowest and the highest
        latitudes look closer on the other side of the earth: */

        interval = (*latitudes.begin() + 180.0) + (180.0 - *latitudes.rbegin());

        if (interval > largestInterval)
        {
            bounds.west = *latitudes.rbegin();
            bounds.east = *latitudes.begin();
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
                bounds.south = *prevIter;
                bounds.north = *iter;
            }

            prevIter = iter;
        }

        /* additionally, check whether the lowest and the highest
           longitudes look closer on the other side of the earth: */

        interval = (*longitudes.begin() + 180.0) + (180.0 - *longitudes.rbegin());

        if (interval > largestInterval)
        {
            bounds.south = *longitudes.rbegin();
            bounds.north = *longitudes.begin();
        }

        /* at this point we have the largest empty view of the earth, so
           if we take the opposite view (from the other side of the earth),
           we have the view that covers all the scanned positions: */

        BasicGeoposition northwestCorner;
        northwestCorner.Altitude = 0.0;
        northwestCorner.Latitude = bounds.south; // swap south & north
        northwestCorner.Latitude = bounds.east; // swap west & east

        BasicGeoposition southeastCorner;
        southeastCorner.Altitude = 0.0;
        southeastCorner.Latitude = bounds.north; // swap south & north
        southeastCorner.Longitude = bounds.west; // swap west & east

        // finally, add some extra to the boundaries, so the content does not touch the border:

        double latDistFromBorder = std::max(0.05 * (bounds.north - bounds.south), 0.01);
        double lngDistFromBorder = std::max(0.05 * (bounds.east - bounds.west), 0.01);

        northwestCorner.Latitude += latDistFromBorder;
        northwestCorner.Longitude -= lngDistFromBorder;
        southeastCorner.Latitude -= latDistFromBorder;
        southeastCorner.Longitude += lngDistFromBorder;

        return ref new GeoboundingBox(northwestCorner, southeastCorner);
    }


    /// <summary>
    /// Merges several views (of routes for the same start and end location)
    /// into one, whose boundaries contain all the others.
    /// </summary>
    /// <param name="viewsBounds">The boundaries of the given views.</param>
    /// <returns>
    /// A <see cref="Windows::Devices::Geolocation::GeoboundingBox"/> object representing
    /// the boundaries for one view that contains all the given ones.
    /// </returns>
    GeoboundingBox ^MergeViewsBoundaries(const std::vector<GeoboundingBox ^> &viewsBounds)
    {
        _ASSERTE(!viewsBounds.empty()); // cannot be empty

        if (viewsBounds.size() == 1)
            return viewsBounds[0];

        // stores the range of latitude and longitude that the merged view must cover
        struct {
            double loLatitude, hiLatitude;
            double loLongitude, hiLongitude;
        } bounds;

        bounds.loLatitude = bounds.loLongitude = +180.0;
        bounds.hiLatitude = bounds.hiLongitude = -180.0;

        /* Because all the views are of routes for the same start and end location,
           they are expected to be quite close to one each other. Also, the analysis
           of whether the view is narrower from the opposite side of the earth has
           been done already. Therefore, the algorithm here simpy produces a bigger
           box that contains all the views. */

        for (auto view : viewsBounds)
        {
            if (view->NorthwestCorner.Latitude > bounds.hiLatitude)
                bounds.hiLatitude = view->NorthwestCorner.Latitude;

            if (view->NorthwestCorner.Longitude < bounds.loLongitude)
                bounds.loLongitude = view->NorthwestCorner.Longitude;

            if (view->SoutheastCorner.Latitude < bounds.loLatitude)
                bounds.loLatitude = view->SoutheastCorner.Latitude;

            if (view->SoutheastCorner.Longitude > bounds.hiLongitude)
                bounds.hiLongitude = view->SoutheastCorner.Longitude;
        }

        BasicGeoposition northwest;
        northwest.Altitude = 0.0;
        northwest.Latitude = bounds.hiLatitude;
        northwest.Longitude = bounds.loLongitude;

        BasicGeoposition southeast;
        southeast.Altitude = 0.0;
        southeast.Latitude = bounds.loLatitude;
        southeast.Longitude = bounds.hiLongitude;

        return ref new GeoboundingBox(northwest, southeast);
    }

}// end of namespace RoutesOverBingMapsApp