#pragma once

#include <vector>
#include <iterator>
#include <functional>
#include <string>

#undef min
#undef max


namespace RoutesOverBingMapsApp
{
    using namespace Windows::Devices::Geolocation;


    /// <summary>
    /// Functor for comparison of <see cref="Windows::Devices::Geolocation::BasicGeoposition" /> value types.
    /// </summary>
    /// <remarks>
    /// This is required by <see cref="Platform::Collections::Vector"/>, otherwise compilation error C2678 is issued.
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

    std::wstring ToTimeSpanText(uint32_t seconds);

    std::wstring ToDistanceText(uint32_t meters);

    GeoboundingBox ^CalculateViewBoundaries(const std::vector<BasicGeoposition> &positions);


    /// <summary>
    /// Merges several views (of routes for the same start and end location)
    /// into one, whose boundaries contain all the others.
    /// </summary>
    /// <param name="begin">
    /// The begin iterator for a range in a collection of objects that can produce
    /// a <see cref="Windows::Devices::Geolocation::GeoboundingBox"/> object.
    /// </param>
    /// <param name="end">
    /// The end iterator for a range in a collection of objects that can produce
    /// a <see cref="Windows::Devices::Geolocation::GeoboundingBox"/> object.
    /// </param>
    /// <param name="getBounds">
    /// A functor that takes an object from the given collection range and produces the
    /// expected <see cref="Windows::Devices::Geolocation::GeoboundingBox"/> object.
    /// </param>
    /// <returns>
    /// A <see cref="Windows::Devices::Geolocation::GeoboundingBox"/> object representing
    /// the boundaries for one view that contains all the given ones.
    /// </returns>
    template <typename Type, typename Iterable>
    GeoboundingBox ^MergeViewsBoundaries(Iterable begin,
                                         Iterable end,
                                         const std::function<GeoboundingBox ^ (Type)> &getBounds)
    {
        _ASSERTE(begin != end); // cannot be empty

        // stores the range of latitude and longitude that the merged view must cover
        struct {
            double loLatitude, hiLatitude;
            double loLongitude, hiLongitude;
        } bounds;

        bounds.loLatitude = +90.0;
        bounds.hiLatitude = -90.0;
        bounds.loLongitude = +180.0;
        bounds.hiLongitude = -180.0;

        /* Because all the views are of routes for the same start and end location,
           they are expected to be quite close to one each other. Also, the analysis
           of whether the view is narrower from the opposite side of the earth has
           been done already. Therefore, the algorithm here simpy produces a bigger
           box that contains all the views. */

        for (auto iter = begin; iter != end; ++iter)
        {
            auto view = getBounds(*iter);

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


    /// <summary>
    /// Merges several views (of routes for the same start and end location)
    /// into one, whose boundaries contain all the others.
    /// </summary>
    /// <param name="begin">
    /// The begin iterator for a range in a collection of objects that can produce
    /// a <see cref="Windows::Devices::Geolocation::GeoboundingBox"/> object.
    /// </param>
    /// <param name="end">
    /// The end iterator for a range in a collection of objects that can produce
    /// a <see cref="Windows::Devices::Geolocation::GeoboundingBox"/> object.
    /// </param>
    /// <returns>
    /// A <see cref="Windows::Devices::Geolocation::GeoboundingBox"/> object representing
    /// the boundaries for one view that contains all the given ones.
    /// </returns>
    template <typename Iterable>
    GeoboundingBox ^MergeViewsBoundaries(Iterable begin, Iterable end)
    {
        return MergeViewsBoundaries<GeoboundingBox ^>(begin, end, [](GeoboundingBox ^entry) { return entry; });
    }

} // end of namespace RoutesOverBingMapsApp