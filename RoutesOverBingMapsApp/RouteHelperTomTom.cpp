#include "pch.h"
#include "RouteHelper.h"
#include "Utils.h"
#include <3FD\exceptions.h>
#include <3FD\callstacktracer.h>
#include <3FD\configuration.h>
#include <3FD\utils_algorithms.h>
#include <3FD\utils_io.h>
#include <array>
#include <cstdio>
#include <ctime>
#include <string>
#include <sstream>
#include <codecvt>
#include <cinttypes>
#include <rapidxml.hpp>
#include <cpprest/http_client.h>

#define format _3fd::utils::FormatArg


namespace RoutesOverBingMapsApp
{
    using namespace _3fd;
    using namespace Windows::Services;


    //////////////////////////////////
    // TomTom Online Routing API
    //////////////////////////////////


    /// <summary>
    /// Holds data obtained by parsing the portion of JSON response from
    /// TomTom Online Routing API that describes a leg in a route.
    /// </summary>
    class RouteLegFromTomTomApi : public IRouteLegFWApi
    {
    private:

        std::vector<BasicGeoposition> m_positions;

    public:

        virtual ~RouteLegFromTomTomApi() {}

        /// <summary>
        /// Parses JSON for a leg in a route returned by TomTom Online Routing API.
        /// </summary>
        /// <param name="jsonLegObj">The JSON for a leg.</param>
        virtual void ParseFromJSON(web::json::value &jsonLegObj) override
        {
            CALL_STACK_TRACE;

            try
            {
                web::json::array points = jsonLegObj.at(L"points").as_array();

                m_positions.reserve(points.size());

                for (auto &entry : points)
                {
                    BasicGeoposition pos;
                    pos.Altitude = 0.0;
                    pos.Latitude = entry.at(L"latitude").as_double();
                    pos.Longitude = entry.at(L"longitude").as_double();
                    m_positions.push_back(pos);
                }
            }
            catch (core::IAppException &)
            {
                throw; // just forward exceptions for errors that have already been handled
            }
            catch (web::json::json_exception &ex)
            {
                std::ostringstream oss;
                oss << "Failed to parse leg JSON response from TomTom Online Routing API: " << ex.what();
                throw core::AppException<std::runtime_error>(oss.str());
            }
            catch (std::exception &ex)
            {
                std::ostringstream oss;
                oss << "Generic failure when parsing leg JSON response from TomTom Online Routing API: " << ex.what();
                throw core::AppException<std::runtime_error>(oss.str());
            }
            catch (Platform::Exception ^ex)
            {
                std::wstring_convert<std::codecvt_utf8<wchar_t>> transcoder;
                std::ostringstream oss;
                oss << "Generic failure when parsing leg JSON response from TomTom Online Routing API: "
                    << transcoder.to_bytes(ex->Message->Data());

                throw core::AppException<std::runtime_error>(oss.str());
            }
        }

        /// <summary>
        /// Appends the geographic positions that compose this route leg to the route path.
        /// </summary>
        /// <param name="geoPath">The vector to receive the geographic
        /// positions for composal of the route path.</param>
        virtual void AppendToPath(std::vector<BasicGeoposition> &geoPath) const override
        {
            geoPath.insert(geoPath.end(),
                           m_positions.begin(),
                           m_positions.end());
        }
    };


    /// <summary>
    /// Holds data obtained by parsing the JSON
    /// response from TomTom Online Routing API.
    /// </summary>
    class RouteFromTomTomApi : public IRouteFromWebAPI
    {
    private:

        std::wstring m_copyright;

    public:

        /// <summary>
        /// Initializes a new instance of the <see cref="RouteFromTomTomApi" /> class.
        /// </summary>
        /// <param name="copyright">
        /// The copyright text, which is passed as a parameter here because
        /// in JSON response from TomTom comes outside the route object.
        /// </param>
        RouteFromTomTomApi(const std::wstring &copyright)
            : m_copyright(copyright) {}

        virtual ~RouteFromTomTomApi() {}

        /// <summary>
        /// Parses JSON for a route returned by TomTom Online Routing API.
        /// </summary>
        /// <param name="jsonRouteObj">The JSON for a route.</param>
        virtual void ParseFromJSON(web::json::value &jsonRouteObj) override
        {
            CALL_STACK_TRACE;

            try
            {
                web::json::array legs = jsonRouteObj.at(L"legs").as_array();

                m_legs.reserve(legs.size());

                for (auto &entry : legs)
                {
                    std::unique_ptr<IRouteLegFWApi> leg(new RouteLegFromTomTomApi());
                    leg->ParseFromJSON(entry);
                    m_legs.push_back(std::move(leg));
                }

                auto summary = jsonRouteObj.at(L"summary");

                uint32_t durationSecs = summary.at(L"travelTimeInSeconds").as_integer();
                uint32_t distanceMeters = summary.at(L"lengthInMeters").as_integer();

                std::wostringstream woss;
                woss << ToTimeSpanText(durationSecs) << L" (" << ToDistanceText(distanceMeters) << L")";

                m_mainInfo = ref new Platform::String(woss.str().c_str());

                woss.str(L"");

                uint32_t liveTrafficSecs = summary.at(L"liveTrafficIncidentsTravelTimeInSeconds").as_integer();
                uint32_t histTrafficSecs = summary.at(L"historicTrafficTravelTimeInSeconds").as_integer();
                uint32_t noTrafficSecs = summary.at(L"noTrafficTravelTimeInSeconds").as_integer();

                woss << L"Travel time wihtout traffic = " << ToTimeSpanText(noTrafficSecs)
                     << L"\nWith historical traffic data = " << ToTimeSpanText(histTrafficSecs)
                     << L"\nWith live traffic data = " << ToTimeSpanText(liveTrafficSecs);

                bool hasToll(false), hasFerry(false);

                web::json::array sections = jsonRouteObj.at(L"sections").as_array();

                for (auto &entry : sections)
                {
                    std::wstring type = entry.at(L"sectionType").as_string();
                    hasToll = (type == L"TOLL_ROAD");
                    hasFerry = (type == L"FERRY");
                }

                if (hasToll)
                    woss << L"\nThis route has tolls!";

                if (hasFerry)
                    woss << L"\nThis route has ferries!";

                woss << L'\n' << m_copyright;

                m_moreInfo = ref new Platform::String(woss.str().c_str());
            }
            catch (core::IAppException &)
            {
                throw; // just forward exceptions for errors that have already been handled
            }
            catch (web::json::json_exception &ex)
            {
                std::ostringstream oss;
                oss << "Failed to parse route JSON response from TomTom Online Routing API: " << ex.what();
                throw core::AppException<std::runtime_error>(oss.str());
            }
            catch (std::exception &ex)
            {
                std::ostringstream oss;
                oss << "Generic failure when parsing route JSON response from TomTom Online Routing API: " << ex.what();
                throw core::AppException<std::runtime_error>(oss.str());
            }
            catch (Platform::Exception ^ex)
            {
                std::wstring_convert<std::codecvt_utf8<wchar_t>> transcoder;
                std::ostringstream oss;
                oss << "Generic failure when parsing route JSON response from TomTom Online Routing API: "
                    << transcoder.to_bytes(ex->Message->Data());

                throw core::AppException<std::runtime_error>(oss.str());
            }
        }
    };


    /// <summary>
    /// Handles the HTTP error responded by TomTom Online Routing API.
    /// </summary>
    /// <param name="response">The response.</param>
    static void HandleHttpError(web::http::http_response &response)
    {
        _ASSERTE(response.status_code() != web::http::status_codes::OK);

        struct KVPair
        {
            web::http::status_code code;
            const char *message;

            web::http::status_code GetKey() const { return code; }
        };

        static std::array<KVPair, 12> errors =
        {
            KVPair { 400, "Bad request: one or more parameters were incorrectly specified, are mutually "
                          "exclusive, the points in the request are not connected by the road network "
                          "or the points in the in the request are not near enough to a road." },
            KVPair { 403, "Permission, capacity, or authentication issues: Forbidden, Not "
                          "authorized, Account inactive, Account over queries per second "
                          "limit, Account over rate limit, Rate limit exceeded" },
            KVPair { 404, "Not Found: the requested resource could not be found, "
                          "but it may be available again in the future." },
            KVPair { 405, "Method Not Allowed: the client used a HTTP method other than GET or POST." },
            KVPair { 408, "Request timeout." },
            KVPair { 414, "Requested uri is too long." },
            KVPair { 500, "An error occurred while processing the request. Please try again later." },
            KVPair { 502, "Internal network connectivity issue." },
            KVPair { 503, "Service currently unavailable." },
            KVPair { 504, "Internal network connectivity issue or a request that has taken too long to complete." },
            KVPair { 596, "Service not found." }
        };

        const char *mainErrMessage("HTTP request to TomTom Online Routing API has failed!");

        try
        {
            std::ostringstream oss;
            
            if (!response.reason_phrase().empty())
                oss << utility::conversions::to_utf8string(response.reason_phrase()) << std::endl;

            oss << "HTTP " << response.status_code() << " - ";

            auto iter = utils::BinarySearch(response.status_code(), errors.begin(), errors.end());

            if (errors.end() != iter)
                oss << iter->message;
            else
                oss << "UNEXPECTED";

            // Parse JSON response:
            if (response.headers().content_type() == L"application/json;charset=utf-8")
            {
                auto jsonBody = response.extract_json().get();

                oss << "\n\n" << utility::conversions::to_utf8string( jsonBody.at(L"error").at(L"description").as_string() );
            }
            // Parse XML response:
            else if (response.headers().content_type() == L"application/xml;charset=utf-8")
            {
                std::string xmlContent = response.extract_utf8string().get();

                rapidxml::xml_document<char> dom;

                dom.parse<rapidxml::parse_non_destructive>((char *)xmlContent.data());

                bool okay(false);

                rapidxml::xml_attribute<char> *attr(nullptr);
                rapidxml::xml_node<char> *node;

                if ((node = dom.first_node("calculateRouteResponse")) != nullptr)
                    if ((node = node->first_node("error")) != nullptr)
                        if ((attr = node->first_attribute("description")) != nullptr)
                            okay = true;
                
                if (okay)
                {
                    oss << "\n\n" << std::string(attr->value(), attr->value() + attr->value_size());
                }
                else
                {
                    oss << "\n\nSecondary failure parsing XML response prevented the retrieval of further details "
                           "about the HTTP error: could not find '/calculateRouteResponse/error/@description'";
                }
            }
            // Unexpected content type:
            else
            {
                oss << "\n\nFurther details from error could not be retrieved because the content type in the HTTP response is unexpected: "
                    << utility::conversions::to_utf8string( response.headers().content_type() );
            }
            
            throw core::AppException<std::runtime_error>(mainErrMessage, oss.str());
        }
        catch (core::IAppException &)
        {
            throw; // prepared exception must be allowed to escape
        }
        catch (web::json::json_exception &ex)
        {
            std::ostringstream oss;
            oss << "Secondary failure in JSON parsing of response prevented the retrieval of further details about the HTTP error: " << ex.what();
            throw core::AppException<std::runtime_error>(mainErrMessage, oss.str());
        }
        catch (std::exception &ex)
        {
            std::ostringstream oss;
            oss << "Secondary failure prevented the retrieval of further details about the HTTP error: " << ex.what();
            throw core::AppException<std::runtime_error>(mainErrMessage, oss.str());
        }
        catch (Platform::Exception ^ex)
        {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> transcoder;

            std::ostringstream oss;
            oss << "\nSecondary failure in XML parsing of response prevented the retrieval of further details about the HTTP error: "
                << transcoder.to_bytes(ex->Message->Data());

            throw core::AppException<std::runtime_error>(mainErrMessage, oss.str());
        }
    }


    /// <summary>
    /// Get the best result for the provided waypoints and requesting to Google Maps API.
    /// </summary>
    /// <param name="waypoints">The required waypoints.</param>
    /// <param name="requiredTime">When not null, is the required departure OR arrival time.</param>
    /// <param name="restrictions">The restrictions to apply in the result.</param>
    /// <returns>The routes returned from TomTom Online Routing API.</returns>
    task<RoutesFromWebApi> GetRoutesFromTomTomAsync(Collections::IVectorView<Waypoint ^> ^waypoints,
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

            client.reset(new http_client(L"https://api.tomtom.com/routing/1/calculateRoute/"));

            std::wostringstream woss;
            woss << L'/';

            auto iter = begin(waypoints);
            
            while (true)
            {
                Waypoint ^entry = *iter;
                BasicGeoposition &position = entry->GetLocation().coordinates;

                woss << position.Latitude << L',' << position.Longitude;

                if (++iter != end(waypoints))
                    woss << L':';
                else
                    break;
            }
            
            woss << L"/json";

            uri.reset(new uri_builder(woss.str()));

            woss.str(L"");

            uri->append_query(L"maxAlternatives", 2);
            uri->append_query(L"sectionType", L"ferry");
            uri->append_query(L"sectionType", L"tollRoad");
            uri->append_query(L"sectionType", L"travelMode");
            uri->append_query(L"computeTravelTimeFor", L"all");

            std::array<wchar_t, 32> buffer;

            // Time requirement:
            if (requiredTime != nullptr)
            {
                wcsftime(buffer.data(),
                         buffer.size(),
                         L"%Y-%m-%dT%H:%M:%S",
                         gmtime(&requiredTime->epochTime));

                switch (requiredTime->type)
                {
                case TimeRequirement::Arrival:
                    uri->append_query(L"arriveAt", buffer.data());
                    break;

                case TimeRequirement::Departure:
                    uri->append_query(L"departAt", buffer.data());
                    break;

                default:
                    break;
                }
            }

            if (static_cast<uint8> (RouteRestriction::AvoidFerries) & restrictions)
                uri->append_query(L"avoid", L"ferries");

            if (static_cast<uint8> (RouteRestriction::AvoidTolls) & restrictions)
                uri->append_query(L"avoid", L"tollRoads");

            if (static_cast<uint8> (RouteRestriction::AvoidDirt) & restrictions)
                uri->append_query(L"avoid", L"unpavedRoads");

            std::string key = core::AppConfig::GetSettings().application.GetString("tomtomApiKey", "NOT SET");

            uri->append_query(L"key", utility::conversions::to_utf16string(key));
        }
        catch (std::exception &ex)
        {
            std::ostringstream oss;
            oss << "Generic failure when assembling request to TomTom Online Routing API: " << ex.what();
            throw core::AppException<std::runtime_error>(oss.str());
        }

        /////////////////////////
        // Issue HTTP request:

        return client->request(http::methods::GET, uri->to_string())
        .then([](http::http_response &response)
        {
            CALL_STACK_TRACE;
            
            if (response.status_code() != http::status_codes::OK)
                HandleHttpError(response);

            return response.extract_json();

        }, task_continuation_context::use_arbitrary())
        .then([](web::json::value &jsonBody)
        {
            CALL_STACK_TRACE;

            /////////////////////////////////////
            // Parse TomTom API JSON response

            try
            {
                auto error = jsonBody[L"error"];

                // first check status:
                if (!error.is_null())
                {
                    std::ostringstream oss;
                    oss << "Request to TomTom Online Routing API returned error: "
                        << utility::conversions::to_utf8string( error.at(L"description").as_string() );

                    throw core::AppException<std::runtime_error>(oss.str());
                }

                std::wstring copyright = jsonBody.at(L"copyright").as_string();
                copyright = copyright.substr(0, copyright.find(L'.'));

                web::json::array jsonRoutes = jsonBody.at(L"routes").as_array();

                RoutesFromWebApi result(new std::vector<std::unique_ptr<IRouteFromWebAPI>>());
                result->reserve(jsonRoutes.size());

                // parse the routes:
                for (auto &entry : jsonRoutes)
                {
                    std::unique_ptr<IRouteFromWebAPI> route(new RouteFromTomTomApi(copyright));
                    route->ParseFromJSON(entry);
                    result->emplace_back(route.release());
                }

                return result;
            }
            catch (core::IAppException &)
            {
                throw; // just forward exceptions for errors that have already been handled
            }
            catch (web::json::json_exception &ex)
            {
                std::ostringstream oss;
                oss << "Failed to parse JSON response from TomTom Online Routing API: " << ex.what();
                throw core::AppException<std::runtime_error>(oss.str());
            }
            catch (std::exception &ex)
            {
                std::ostringstream oss;
                oss << "Generic failure in client for TomTom Online Routing API: " << ex.what();
                throw core::AppException<std::runtime_error>(oss.str());
            }
            catch (Platform::Exception ^ex)
            {
                std::wstring_convert<std::codecvt_utf8<wchar_t>> transcoder;
                std::ostringstream oss;
                oss << "Generic failure in client for TomTom Online Routing API: "
                    << transcoder.to_bytes(ex->ToString()->Data());

                throw core::AppException<std::runtime_error>(oss.str());
            }

        }, task_continuation_context::use_arbitrary());
    }

}// end of namespace RoutesOverBingMapsApp
