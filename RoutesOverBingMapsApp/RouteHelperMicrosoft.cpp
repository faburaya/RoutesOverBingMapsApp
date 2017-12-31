#include "pch.h"
#include "RouteHelper.h"
#include "Utils.h"
#include <3FD\exceptions.h>
#include <3FD\callstacktracer.h>
#include <array>
#include <string>
#include <sstream>
#include <codecvt>
#include <cinttypes>


namespace RoutesOverBingMapsApp
{
    using namespace _3fd;
    using namespace Windows::Services;


    /////////////////////////////////////
    // Microsoft WinRT API for Maps
    /////////////////////////////////////


    /// <summary>
    /// Converts route restrictions flags to the type expected by <see cref="Windows::Services::Maps"/>.
    /// </summary>
    /// <param name="restrictions">The route restrictions.</param>
    /// <returns>Value converted to <see cref="Windows::Services::Maps::MapRouteRestrictions"/>.</returns>
    static MapRouteRestrictions ConvertToMicrosoftType(uint8 restrictions)
    {
        auto result = static_cast<uint32> (MapRouteRestrictions::None);

        if (static_cast<uint8> (RouteRestriction::AvoidFerries) & restrictions)
            result |= static_cast<uint32> (MapRouteRestrictions::Ferries);

        if (static_cast<uint8> (RouteRestriction::AvoidDirt) & restrictions)
            result |= static_cast<uint32> (MapRouteRestrictions::DirtRoads);

        if (static_cast<uint8> (RouteRestriction::AvoidTolls) & restrictions)
            result |= static_cast<uint32> (MapRouteRestrictions::TollRoads);

        if (static_cast<uint8> (RouteRestriction::AvoidHighways) & restrictions)
            result |= static_cast<uint32> (MapRouteRestrictions::Highways);

        return static_cast<MapRouteRestrictions> (result);
    }


    /// <summary>
    /// Produces a description for a value <see cref="Windows::Services::MapRouteFinderStatus"/>.
    /// </summary>
    /// <param name="status">The status code returned by <see cref="Windows::Services::Maps::MapRouteFinder"/>.</param>
    /// <returns>A description for the status code.</returns>
    static const char *ToString(MapRouteFinderStatus status)
    {
        switch (status)
        {
        case MapRouteFinderStatus::Success:
            return "The query was successful.";

        case MapRouteFinderStatus::UnknownError:
            return "The query returned an unknown error.";

        case MapRouteFinderStatus::InvalidCredentials:
            return "The query provided credentials that are not valid.";

        case MapRouteFinderStatus::NoRouteFound:
            return "The query did not find a route.";

        case MapRouteFinderStatus::NoRouteFoundWithGivenOptions:
            return "The query did not find a route with the specified options.";

        case MapRouteFinderStatus::StartPointNotFound:
            return "The specified starting point is not valid in a route. For example, the point is in an ocean or a desert.";

        case MapRouteFinderStatus::EndPointNotFound:
            return "The specified ending point is not valid in a route. For example, the point is in an ocean or a desert.";

        case MapRouteFinderStatus::NoPedestrianRouteFound:
            return "The query did not find a pedestrian route.";

        case MapRouteFinderStatus::NetworkFailure:
            return "The query encountered a network failure.";

        case MapRouteFinderStatus::NotSupported:
            return "The query is not supported.";

        default:
            return "UNKNOWN STATUS";
        }
    }


    /// <summary>
    /// Get the best result for the provided waypoints using the
    /// given transportation and requesting to Bing Maps service.
    /// </summary>
    /// <param name="waypoints">The required waypoints.</param>
    /// <param name="optimization">What to optimize the result for.</param>
    /// <param name="restrictions">The restrictions to apply in the result.</param>
    /// <returns>The result returned from <see cref="Windows::Services::Maps::MapRouteFinder"/>.</returns>
    task<IVector<MapRoute ^> ^> GetRoutesFromMicrosoftAsync(Collections::IVectorView<Waypoint ^> ^waypoints,
                                                            MapRouteOptimization optimization,
                                                            uint8 restrictions)
    {
        auto geopoints = ref new Platform::Collections::Vector<Geopoint ^>();

        for (auto &&waypt : waypoints)
        {
            geopoints->Append(
                ref new Geopoint(waypt->GetLocation().coordinates)
            );
        }

        return create_task(
            MapRouteFinder::GetDrivingRouteFromWaypointsAsync(geopoints, optimization, ConvertToMicrosoftType(restrictions))
        )
        .then([](MapRouteFinderResult ^result)
        {
            CALL_STACK_TRACE;

            // failed? notify user and return nothing
            if (result->Status != MapRouteFinderStatus::Success)
            {
                std::ostringstream oss;
                oss << "Failed to find route using WinRT Maps Services: " << ToString(result->Status);
                throw core::AppException<std::runtime_error>(oss.str());
            }

            auto routes = ref new Platform::Collections::Vector<MapRoute ^>();

            routes->Append(result->Route);

            for (auto &&entry : result->AlternateRoutes)
                routes->Append(entry);

            return static_cast<IVector<MapRoute ^> ^> (routes);

        }, task_continuation_context::use_arbitrary());
    }

}// end of namespace RoutesOverBingMapsApp
