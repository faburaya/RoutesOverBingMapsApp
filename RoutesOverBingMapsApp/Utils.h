#pragma once

#include <vector>

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

    GeoboundingBox ^CalculateViewBoundaries(const std::vector<BasicGeoposition> &positions);

    GeoboundingBox ^MergeViewsBoundaries(const std::vector<GeoboundingBox ^> &viewsBounds);

} // end of namespace RoutesOverBingMapsApp