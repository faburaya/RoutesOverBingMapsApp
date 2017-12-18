#include "pch.h"
#include "RouteHelper.h"
#include <3FD\exceptions.h>
#include <3FD\callstacktracer.h>
#include <3FD\utils_winrt.h>
#include <3FD\utils_io.h>
#include <array>
#include <cstdio>
#include <string>
#include <sstream>
#include <cpprest/http_client.h>
#include <cpprest/json.h>

#define format _3fd::utils::FormatArg


namespace RoutesOverBingMapsApp
{
    using namespace _3fd;
    using namespace Windows::Services;


    // default parameters for exception notification and logging
    static const _3fd::utils::UwpXaml::ExNotifAndLogParams exHndParams
    {
        Platform::StringReference(L"Application error!\n"),
        Platform::StringReference(L"Cancel"),
        _3fd::core::Logger::PRIO_ERROR
    };


    ////////////////
    // Microsoft
    ////////////////

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
    static Platform::String ^ToString(MapRouteFinderStatus status)
    {
        switch (status)
        {
        case MapRouteFinderStatus::Success:
            return Platform::StringReference(L"The query was successful.");

        case MapRouteFinderStatus::UnknownError:
            return Platform::StringReference(L"The query returned an unknown error.");

        case MapRouteFinderStatus::InvalidCredentials:
            return Platform::StringReference(L"The query provided credentials that are not valid.");

        case MapRouteFinderStatus::NoRouteFound:
            return Platform::StringReference(L"The query did not find a route.");

        case MapRouteFinderStatus::NoRouteFoundWithGivenOptions:
            return Platform::StringReference(L"The query did not find a route with the specified options.");

        case MapRouteFinderStatus::StartPointNotFound:
            return Platform::StringReference(L"	The specified starting point is not valid in a route.For example, the point is in an ocean or a desert.");

        case MapRouteFinderStatus::EndPointNotFound:
            return Platform::StringReference(L"The specified ending point is not valid in a route.For example, the point is in an ocean or a desert.");

        case MapRouteFinderStatus::NoPedestrianRouteFound:
            return Platform::StringReference(L"The query did not find a pedestrian route.");

        case MapRouteFinderStatus::NetworkFailure:
            return Platform::StringReference(L"The query encountered a network failure.");

        case MapRouteFinderStatus::NotSupported:
            return Platform::StringReference(L"The query is not supported.");

        default:
            return Platform::StringReference(L"UNKNOWN STATUS");
        }
    }


    /// <summary>
    /// Get the best routes for the provided waypoints using the
    /// given transportation and requesting to Bing Maps service.
    /// </summary>
    /// <param name="waypoints">The required waypoints.</param>
    /// <param name="optimization">What to optimize the routes for.</param>
    /// <param name="restrictions">The restrictions to apply in the routes.</param>
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
        .then([](task<MapRouteFinderResult ^> priorTask)
        {
            CALL_STACK_TRACE;

            MapRouteFinderResult ^result;
            utils::UwpXaml::GetTaskRetAndHndEx(priorTask, result, exHndParams);

            if (result->Status != MapRouteFinderStatus::Success)
            {
                utils::UwpXaml::Notify(exHndParams.title,
                                       ToString(result->Status),
                                       exHndParams.closeButtonText);
            }

            auto routes = ref new Platform::Collections::Vector<MapRoute ^>();
            routes->Append(result->Route);

            for (auto &&entry : result->AlternateRoutes)
                routes->Append(entry);

            return static_cast<IVector<MapRoute ^> ^> (routes);
        });
    }


    ////////////////////
    // Google Maps
    ////////////////////


    /// <summary>
    /// Converts route restrictions flags to the type expected by Google Maps API.
    /// </summary>
    /// <param name="restrictions">The route restrictions.</param>
    /// <returns>Value converted to a string.</returns>
    const wchar_t *ConvertToGoogleFormat(uint8 restrictions)
    {
        static std::array<wchar_t, 24> result = { 0 };

        if (restrictions == 0)
            return L"";

        bool moreThanOne(false);

        if (static_cast<uint8> (RouteRestriction::AvoidFerries) & restrictions)
        {
            wcscat(result.data(), L"ferries");
            moreThanOne = true;
        }

        if (static_cast<uint8> (RouteRestriction::AvoidTolls) & restrictions)
        {
            if (moreThanOne)
                wcscat(result.data(), L"|");
            else
                moreThanOne = true;

            wcscat(result.data(), L"tolls");
        }

        if (static_cast<uint8> (RouteRestriction::AvoidHighways) & restrictions)
        {
            if (moreThanOne)
                wcscat(result.data(), L"|");
            else
                moreThanOne = true;

            wcscat(result.data(), L"highways");
        }

        return result.data();
    }


    /// <summary>
    /// Converts the middle waypoints (between origin and destination)
    /// to the format expected by Google Maps API.
    /// </summary>
    /// <param name="restrictions">The route waypoints.</param>
    /// <returns>Values converted to a string.</returns>
    std::wstring ConvertMidWayptsToGoogleFormat(Collections::IVectorView<Waypoint ^> ^waypoints)
    {
        std::wostringstream woss;

        // Waypoints (beyond origin and destination):
        for (uint32 idx = 1; idx < waypoints->Size - 1; ++idx)
        {
            auto &coordinates = waypoints->GetAt(idx)->GetLocation().coordinates;

            if (idx > 1)
                woss << L'|';

            woss << coordinates.Latitude << L',' << coordinates.Longitude;
        }

        return woss.str();
    }


    /// <summary>
    /// Get the best routes for the provided waypoints using the
    /// given transportation and requesting to Google Maps API.
    /// </summary>
    /// <param name="waypoints">The required waypoints.</param>
    /// <param name="requiredTime">When not null, is the required departure OR arrival time.</param>
    /// <param name="restrictions">The restrictions to apply in the routes.</param>
    task<IVector<MapRoute ^> ^> GetRoutesFromGoogleAsync(Collections::IVectorView<Waypoint ^> ^waypoints,
                                                         TimeRequirement *requiredTime,
                                                         uint8 restrictions)
    {
        CALL_STACK_TRACE;

        using namespace web;
        using namespace web::http::client;
        
        _ASSERTE(waypoints->Size > 1);

        http_client client(U("https://maps.googleapis.com/maps/api/directions/"));
        uri_builder uri(U("/json"));

        std::array<wchar_t, 32> buffer;

        // Origin:
        BasicGeoposition &origin = waypoints->GetAt(0)->GetLocation().coordinates;
        utils::SerializeTo(buffer, origin.Latitude, L",", origin.Longitude);
        uri.append_query(U("origin"), buffer.data());

        // Destination:
        BasicGeoposition &destination = waypoints->GetAt(waypoints->Size - 1)->GetLocation().coordinates;
        utils::SerializeTo(buffer, origin.Latitude, L",", origin.Longitude);
        uri.append_query(U("destination"), buffer.data());

        // Time requirement:
        if (requiredTime != nullptr)
        {
            utils::SerializeTo(buffer, requiredTime->epochTime);

            switch (requiredTime->type)
            {
            case TimeRequirement::Arrival:
                uri.append_query(U("arrival_time"), buffer.data());
                break;

            case TimeRequirement::Departure:
                uri.append_query(U("departure_time"), buffer.data());
                break;

            default:
                break;
            }
        }

        // Waypoints between origin and destination:
        uri.append_query(U("waypoints"), ConvertMidWayptsToGoogleFormat(waypoints));

        // Constants:
        uri.append_query(U("mode"),         U("driving"));
        uri.append_query(U("alternatives"), U("true"));
        uri.append_query(U("units"),        U("metric"));
        uri.append_query(U("key"),          U("AIzaSyC-92iQJwDNoEuq5H3lDGhTzEMKOVlzR3s"));

        return client.request(http::methods::GET, uri.to_string())
            .then([](task<http::http_response> priorTask)
            {
                CALL_STACK_TRACE;

                http::http_response response;
                utils::UwpXaml::GetTaskRetAndHndEx(priorTask, response, exHndParams);

                if (response.status_code() != http::status_codes::OK)
                {
                    throw core::AppException<std::runtime_error>(L"HTTP request to Google Maps Directions API has failed!", response.reason_phrase());
                }

                return response.extract_json();

            })
            .then([](task<web::json::value> priorTask)
            {
                CALL_STACK_TRACE;

                web::json::value jsonBody;
                utils::UwpXaml::GetTaskRetAndHndEx(priorTask, jsonBody, exHndParams);

                auto status = jsonBody[U("status")].as_string();

                if (status != U("OK"))
                {
                    std::wostringstream woss;
                    woss << L"Request to Google Maps Directions API returned status '" << status << L'\'';

                    auto errMessage = jsonBody[U("error_message")];
                    if (!errMessage.is_null())
                        woss << L": " << errMessage.as_string();

                    utils::UwpXaml::Notify(exHndParams.title,
                                           ref new Platform::String(woss.str().c_str()),
                                           exHndParams.closeButtonText);
                }

                auto routes = ref new Platform::Collections::Vector<MapRoute ^>();

                // TO DO

                return static_cast<IVector<MapRoute ^> ^> (routes);
            });
    }

}// end of namespace RoutesOverBingMapsApp
