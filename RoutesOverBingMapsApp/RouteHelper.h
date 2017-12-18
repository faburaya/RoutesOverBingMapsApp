#pragma once

#include "AppViewModel.h"
#include <vector>
#include <ctime>

namespace RoutesOverBingMapsApp
{
    using namespace Concurrency;
    using namespace Windows::Foundation;
    using namespace Windows::Services::Maps;

    using Collections::IVector;


    /// <summary>
    /// Enumerates the options for route restrictions.
    /// </summary>
    public enum class RouteRestriction : uint8
    {
        None          = 0x00,
        AvoidFerries  = 0x01,
        AvoidDirt     = 0x02,
        AvoidTolls    = 0x04,
        AvoidHighways = 0x08
    };

    /// <summary>
    /// Holds info about departure or arrival time.
    /// </summary>
    struct TimeRequirement
    {
        enum TimeType { Departure, Arrival };

        time_t epochTime;
        TimeType type;
    };

    task<IVector<MapRoute ^> ^> GetRoutesFromMicrosoftAsync(Collections::IVectorView<Waypoint ^> ^waypoints,
                                                            MapRouteOptimization optimization,
                                                            uint8 restrictions);

    task<IVector<MapRoute ^> ^> GetRoutesFromGoogleAsync(Collections::IVectorView<Waypoint ^> ^waypoints,
                                                         TimeRequirement *requiredTime,
                                                         uint8 restrictions);

}// end of namespace RoutesOverBingMapsApp