#include "pch.h"
#include "RouteHelper.h"
#include "Utils.h"
#include <3FD\exceptions.h>
#include <3FD\callstacktracer.h>
#include <3FD\utils_io.h>
#include <array>
#include <cstdio>
#include <string>
#include <sstream>
#include <codecvt>
#include <cinttypes>
#include <cpprest/http_client.h>
#include <cpprest/json.h>

#define format _3fd::utils::FormatArg


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


    //////////////////////////////////
    // Google Maps Directions API
    //////////////////////////////////


    /// <summary>
    /// Decodes a piece from the polyline corresponding to 1 number.
    /// The input in a group of "5-bit chunks in reverse order".
    /// (Reference: https://developers.google.com/maps/documentation/utilities/polylinealgorithm)
    /// </summary>
    /// <param name="begin">The iterator for the first chunk (5 bits occupying 1 byte).</param>
    /// <param name="end">The iterator for one past the last chunk.</param>
    /// <returns>The decoded number.</returns>
    static double DecodeNumberFromPolyline(std::string::const_iterator begin, std::string::const_iterator end)
    {
        int value(0);
        int chunk(0);

        /* the chunks of 5-bits are in reverse order, so this
           will reassemble the bits in the right order: */
        for (auto iter = begin; iter != end; ++iter)
            value += (*iter << (5 * chunk++));

        /* least significant bit is activated when the original number
           is negative, which means that the value must be inverted */
        if (value & 0x01)
            value = ~value;

        value >>= 1;

        double final = value / 1e5;

        _ASSERTE(final >= -180.0 && final <= 180.0);

        return final;
    }


    /// <summary>
    /// Decodes a polyline encoded by Google Maps Directions API.
    /// (Reference: https://developers.google.com/maps/documentation/utilities/polylinealgorithm)
    /// </summary>
    /// <param name="polyLine">The polyline as an encoded string.</param>
    /// <param name="path">The path: a vector of geo positions to receive the decoded result.</param>
    static void DecodeFromPolyline(const std::string &polyLine, std::vector<BasicGeoposition> &path)
    {
        CALL_STACK_TRACE;

        try
        {
            path.clear();

            std::vector<double> numbers;
            numbers.reserve(polyLine.length() / 4); // reserve approximate room

            std::string code = polyLine;
            auto begin = code.begin();

            // scan the string and separates each group that represent a number (latitude or longitude):
            for (auto iter = begin; iter != code.end(); ++iter)
            {
                uint8_t b = *iter - 63;

                if (b | 0x20 != 0) // has the bit flag, so not at the group's end:
                {
                    *iter = b - 0x20; // remove the flag
                }
                else // end of group:
                {
                    // decode the group into a number:
                    *iter = b;
                    numbers.push_back(DecodeNumberFromPolyline(begin, iter + 1));
                    begin = iter + 1;
                }
            }

            /* positions are always pairs of latitude & longitude, so
               the total of decoded numbers must be pair as well! */
            _ASSERTE(numbers.size() % 2 == 0);

            path.resize(numbers.size() / 2);

            // scan the numbers in pairs:
            for (int idx = 0; idx < numbers.size(); idx += 2)
            {
                auto &position = path[idx / 2];
                position.Altitude = 0.0;
                position.Latitude = numbers[idx];
                position.Longitude = numbers[idx + 1];

                // the first position is absolute and the others are offsets:
                if (idx != 0)
                {
                    auto &firstPos = path[0];
                    position.Latitude += firstPos.Latitude;
                    position.Longitude += firstPos.Longitude;
                }
            }
        }
        catch (std::exception &ex)
        {
            std::ostringstream oss;
            oss << "Generic failure when parsing polyline from Google Maps Directions API: " << ex.what();
            throw core::AppException<std::runtime_error>(oss.str());
        }
    }


    /// <summary>
    /// Holds data obtained by parsing the portion of JSON response from
    /// Google Maps Directions API that describes a maneuver in a route.
    /// </summary>
    class RouteLegManeuverFromGoogleApi : public IRouteLegManeuverFWApi
    {
    private:

        std::vector<BasicGeoposition> m_path;

    public:

        /// <summary>
        /// Parses JSON for a maneuver in a route returned by Google Maps Directions API.
        /// </summary>
        /// <param name="jsonManueverObj">The JSON for a maneuver.</param>
        virtual void ParseFromJSON(web::json::value &jsonManueverObj) override
        {
            CALL_STACK_TRACE;

            try
            {
                std::string polyline =
                    utility::conversions::to_utf8string(
                        jsonManueverObj.at(L"polyline").at(L"points").as_string()
                    );

                DecodeFromPolyline(polyline, m_path);
            }
            catch (web::json::json_exception &ex)
            {
                std::ostringstream oss;
                oss << "Failed to parse maneuver JSON response from Google Maps Directions API: " << ex.what();
                throw core::AppException<std::runtime_error>(oss.str());
            }
            catch (std::exception &ex)
            {
                std::ostringstream oss;
                oss << "Generic failure when parsing maneuver JSON response from Google Maps Directions API: " << ex.what();
                throw core::AppException<std::runtime_error>(oss.str());
            }
        }

        /// <summary>
        /// Appends the geographic positions that compose this maneuver to the route path.
        /// </summary>
        /// <param name="geoPath">The vector to receive the geographic
        /// positions for composal of the route path.</param>
        virtual void AppendToPath(std::vector<BasicGeoposition> &geoPath) override
        {
            geoPath.insert(geoPath.end(), m_path.begin(), m_path.end());
        }
    };


    /// <summary>
    /// Holds data obtained by parsing the portion of JSON response from
    /// Google Maps Directions API that describes a leg in a route.
    /// </summary>
    class RouteLegFromGoogleApi : public IRouteLegFWApi
    {
    public:

        /// <summary>
        /// Parses JSON for a leg in a route returned by Google Maps Directions API.
        /// </summary>
        /// <param name="jsonLegObj">The JSON for a leg.</param>
        virtual void ParseFromJSON(web::json::value &jsonLegObj) override
        {
            CALL_STACK_TRACE;

            try
            {
                auto node = jsonLegObj[L"distance"];

                if (!node.is_null())
                    m_distanceMeters = node.as_integer();

                node = jsonLegObj[L"duration"];

                if (!node.is_null())
                    m_durationSecs = node.as_integer();

                node = jsonLegObj.at(L"start_location");
                m_startPosition.Latitude = node.at(L"lat").as_double();
                m_startPosition.Longitude = node.at(L"lng").as_double();

                node = jsonLegObj.at(L"end_location");
                m_endPosition.Latitude = node.at(L"lat").as_double();
                m_endPosition.Longitude = node.at(L"lng").as_double();

                web::json::array steps = jsonLegObj.at(L"steps").as_array();

                for (auto &entry : steps)
                {
                    auto maneuver = std::make_unique<IRouteLegManeuverFWApi>(new RouteLegManeuverFromGoogleApi());
                    maneuver->ParseFromJSON(entry);
                    m_manuevers.push_back(std::move(maneuver));
                }
            }
            catch (web::json::json_exception &ex)
            {
                std::ostringstream oss;
                oss << "Failed to parse leg JSON response from Google Maps Directions API: " << ex.what();
                throw core::AppException<std::runtime_error>(oss.str());
            }
            catch (std::exception &ex)
            {
                std::ostringstream oss;
                oss << "Generic failure when parsing leg JSON response from Google Maps Directions API: " << ex.what();
                throw core::AppException<std::runtime_error>(oss.str());
            }
            catch (Platform::String ^ex)
            {
                std::wstring_convert<std::codecvt_utf8<wchar_t>> transcoder;
                std::ostringstream oss;
                oss << "Generic failure when parsing leg JSON response from Google Maps Directions API: "
                    << transcoder.to_bytes(ex->ToString()->Data());

                throw core::AppException<std::runtime_error>(oss.str());
            }
        }
    };


    /// <summary>
    /// Holds data obtained by parsing the JSON
    /// response from Google Maps Directions API.
    /// </summary>
    class RouteFromGoogleApi : public IRouteFromWebAPI
    {
    private:

    public:

        /// <summary>
        /// Parses JSON for a route returned by Google Maps Directions API.
        /// </summary>
        /// <param name="jsonRouteObj">The JSON for a route.</param>
        virtual void ParseFromJSON(web::json::value &jsonRouteObj) override
        {
            CALL_STACK_TRACE;

            try
            {
                auto southwest = jsonRouteObj.at(L"bounds").at(L"southwest");
                auto northeast = jsonRouteObj.at(L"bounds").at(L"northeast");

                BasicGeoposition northwest;
                northwest.Altitude = 0.0;
                northwest.Latitude = northeast.at(L"lat").as_double();
                northwest.Longitude = southwest.at(L"lng").as_double();

                BasicGeoposition southeast;
                southeast.Altitude = 0.0;
                southeast.Latitude = southwest.at(L"lat").as_double();
                southeast.Longitude = northeast.at(L"lng").as_double();

                m_boundingBox = ref new GeoboundingBox(northwest, southeast);

                web::json::array legs = jsonRouteObj.at(L"legs").as_array();

                for (auto &entry : legs)
                {
                    auto leg = std::make_unique<IRouteLegFWApi>(new RouteLegFromGoogleApi());
                    leg->ParseFromJSON(entry);
                    m_legs.push_back(std::move(leg));
                }

                std::wostringstream woss;

                auto fare = jsonRouteObj[L"fare"];

                if (!fare.is_null())
                    woss << "Fare: " << fare.at(L"text").as_string() << std::endl;

                web::json::array warnings = jsonRouteObj.at(L"warnings").as_array();

                for (auto &entry : warnings)
                    woss << L"Warning! " << entry.as_string() << std::endl;

                woss << L"Copyrights: " << jsonRouteObj.at(L"copyrights").as_string() << std::endl;
            }
            catch (web::json::json_exception &ex)
            {
                std::ostringstream oss;
                oss << "Failed to parse route JSON response from Google Maps Directions API: " << ex.what();
                throw core::AppException<std::runtime_error>(oss.str());
            }
            catch (std::exception &ex)
            {
                std::ostringstream oss;
                oss << "Generic failure when parsing route JSON response from Google Maps Directions API: " << ex.what();
                throw core::AppException<std::runtime_error>(oss.str());
            }
            catch (Platform::String ^ex)
            {
                std::wstring_convert<std::codecvt_utf8<wchar_t>> transcoder;
                std::ostringstream oss;
                oss << "Generic failure when parsing route JSON response from Google Maps Directions API: "
                    << transcoder.to_bytes(ex->ToString()->Data());

                throw core::AppException<std::runtime_error>(oss.str());
            }
        }
    };


    /// <summary>
    /// Converts route restrictions flags to the type expected by Google Maps API.
    /// </summary>
    /// <param name="restrictions">The route restrictions.</param>
    /// <returns>Value converted to a string.</returns>
    static std::wstring ConvertToGoogleFormat(uint8 restrictions)
    {
        std::wostringstream woss;

        if (restrictions == 0)
            return L"";

        bool moreThanOne(false);

        if (static_cast<uint8> (RouteRestriction::AvoidFerries) & restrictions)
        {
            woss << L"ferries";
            moreThanOne = true;
        }

        if (static_cast<uint8> (RouteRestriction::AvoidTolls) & restrictions)
        {
            if (moreThanOne)
                woss << L'|';
            else
                moreThanOne = true;

            woss << L"tolls";
        }

        if (static_cast<uint8> (RouteRestriction::AvoidHighways) & restrictions)
        {
            if (moreThanOne)
                woss << L'|';
            else
                moreThanOne = true;

            woss << L"highways";
        }

        return woss.str();
    }


    /// <summary>
    /// Converts the middle waypoints (between origin and destination)
    /// to the format expected by Google Maps API.
    /// </summary>
    /// <param name="restrictions">The route waypoints.</param>
    /// <returns>Values converted to a string.</returns>
    static std::wstring ConvertMidWayptsToGoogleFormat(Collections::IVectorView<Waypoint ^> ^waypoints)
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
    /// Get the best result for the provided waypoints using the
    /// given transportation and requesting to Google Maps API.
    /// </summary>
    /// <param name="waypoints">The required waypoints.</param>
    /// <param name="requiredTime">When not null, is the required departure OR arrival time.</param>
    /// <param name="restrictions">The restrictions to apply in the result.</param>
    /// <returns>
    /// The routes returned from Google Maps Directions API converted into
    /// a list of <see cref="Windows::Devices::Geolocation::Geopath"/> objects,
    /// plus the boundaries of a view that contains all paths.
    /// </returns>
    task<ListOfRoutes> GetRoutesFromGoogleAsync(Collections::IVectorView<Waypoint ^> ^waypoints,
                                                TimeRequirement *requiredTime,
                                                uint8 restrictions)
    {
        CALL_STACK_TRACE;

        using namespace web;
        using namespace web::http::client;
        
        _ASSERTE(waypoints->Size > 1);

        std::unique_ptr<http_client> client;
        std::unique_ptr<uri_builder> uri;

        try
        {
            ///////////////////////////
            // Assemble HTTP request:

            client.reset(new http_client(L"https://maps.googleapis.com/maps/api/directions/"));
            uri.reset(new uri_builder(L"/json"));

            std::array<wchar_t, 32> buffer;

            // Origin:
            BasicGeoposition &origin = waypoints->GetAt(0)->GetLocation().coordinates;
            utils::SerializeTo(buffer, origin.Latitude, L",", origin.Longitude);
            uri->append_query(L"origin", buffer.data());

            // Destination:
            BasicGeoposition &destination = waypoints->GetAt(waypoints->Size - 1)->GetLocation().coordinates;
            utils::SerializeTo(buffer, origin.Latitude, L",", origin.Longitude);
            uri->append_query(L"destination", buffer.data());

            // Time requirement:
            if (requiredTime != nullptr)
            {
                utils::SerializeTo(buffer, requiredTime->epochTime);

                switch (requiredTime->type)
                {
                case TimeRequirement::Arrival:
                    uri->append_query(L"arrival_time", buffer.data());
                    break;

                case TimeRequirement::Departure:
                    uri->append_query(L"departure_time", buffer.data());
                    break;

                default:
                    break;
                }
            }

            // Waypoints between origin and destination:
            uri->append_query(L"waypoints", ConvertMidWayptsToGoogleFormat(waypoints));

            // Constants:
            uri->append_query(L"mode", L"driving");
            uri->append_query(L"alternatives", L"true");
            uri->append_query(L"units", L"metric");
            uri->append_query(L"key", L"AIzaSyC-92iQJwDNoEuq5H3lDGhTzEMKOVlzR3s");
        }
        catch (std::exception &ex)
        {
            std::ostringstream oss;
            oss << "Generic failure when assembling request to Google Maps Directions API: " << ex.what();
            throw core::AppException<std::runtime_error>(oss.str());
        }

        /////////////////////////
        // Issue HTTP request:

        return client->request(http::methods::GET, uri->to_string())
        .then([](http::http_response &response)
        {
            CALL_STACK_TRACE;

            if (response.status_code() != http::status_codes::OK)
            {
                std::wstring_convert<std::codecvt_utf8<wchar_t>> transcoder;

                throw core::AppException<std::runtime_error>(
                    "HTTP request to Google Maps Directions API has failed!",
                    transcoder.to_bytes(response.reason_phrase())
                );
            }

            return response.extract_json();

        }, task_continuation_context::use_arbitrary())
        .then([](web::json::value &jsonBody)
        {
            CALL_STACK_TRACE;

            ///////////////////////////////////////////////
            // Transform JSON into a list of MapRoute's:

            try
            {
                ListOfRoutes result;

                auto status = jsonBody.at(L"status").as_string();

                // first check status:
                if (status != L"OK")
                {
                    std::ostringstream oss;
                    oss << "Request to Google Maps Directions API returned status '"
                        << utility::conversions::to_utf8string(status) << L'\'';

                    auto errMessage = jsonBody[L"error_message"];

                    if (!errMessage.is_null())
                        oss << ": " << utility::conversions::to_utf8string(errMessage.as_string());

                    throw core::AppException<std::runtime_error>(oss.str());
                }

                web::json::array jsonRoutes = jsonBody.at(L"routes").as_array();

                std::vector<GeoboundingBox ^> viewsBounds;
                viewsBounds.reserve(jsonRoutes.size());

                std::vector<Geopath ^> routes;
                routes.reserve(jsonRoutes.size());

                // parse the routes:
                for (auto &entry : jsonRoutes)
                {
                    std::vector<BasicGeoposition> path;

                    auto route = std::make_unique<IRouteFromWebAPI>(new RouteFromGoogleApi());
                    route->ParseFromJSON(entry);
                    route->GeneratePath(path);

                    routes.push_back(
                        ref new Geopath(
                            ref new Platform::Collections::Vector<BasicGeoposition, BGeoPosComparator>(std::move(path))
                        )
                    );

                    viewsBounds.push_back(route->GetBounds());
                }

                result.boundaries = MergeViewsBoundaries(viewsBounds);
                result.routes = ref new Platform::Collections::Vector<Geopath ^>(std::move(routes));

                return result;
            }
            catch (core::IAppException &ex)
            {
                throw; // just forward exceptions for errors that have already been handled
            }
            catch (web::json::json_exception &ex)
            {
                std::ostringstream oss;
                oss << "Failed to parse JSON response from Google Maps Directions API: " << ex.what();
                throw core::AppException<std::runtime_error>(oss.str());
            }
            catch (std::exception &ex)
            {
                std::ostringstream oss;
                oss << "Generic failure in client for Google Maps Directions API: " << ex.what();
                throw core::AppException<std::runtime_error>(oss.str());
            }
            catch (Platform::Exception ^ex)
            {
                std::wstring_convert<std::codecvt_utf8<wchar_t>> transcoder;
                std::ostringstream oss;
                oss << "Generic failure in client for Google Maps Directions API: "
                    << transcoder.to_bytes(ex->ToString()->Data());

                throw core::AppException<std::runtime_error>(oss.str());
            }

        }, task_continuation_context::use_arbitrary());
    }

}// end of namespace RoutesOverBingMapsApp
