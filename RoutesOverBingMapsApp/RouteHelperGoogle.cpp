#include "pch.h"
#include "RouteHelper.h"
#include "Utils.h"
#include <3FD\exceptions.h>
#include <3FD\callstacktracer.h>
#include <3FD\configuration.h>
#include <3FD\utils_io.h>
#include <array>
#include <cstdio>
#include <ctime>
#include <string>
#include <sstream>
#include <codecvt>
#include <cinttypes>
#include <cpprest/http_client.h>

#define format _3fd::utils::FormatArg


namespace RoutesOverBingMapsApp
{
    using namespace _3fd;
    using namespace Windows::Services;


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

                if ((b & 0x20) != 0) // has the bit flag, so not at the group's end:
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
                    auto &prevPos = path[(idx / 2) - 1];
                    position.Latitude += prevPos.Latitude;
                    position.Longitude += prevPos.Longitude;
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

        virtual ~RouteLegManeuverFromGoogleApi() {}

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
            catch (core::IAppException &)
            {
                throw; // just forward exceptions for errors that have already been handled
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
    private:

        std::vector<std::unique_ptr<IRouteLegManeuverFWApi>> m_manuevers;

        BasicGeoposition m_startPosition;
        BasicGeoposition m_endPosition;
        uint32_t m_durationSecs;
        uint32_t m_distanceMeters;

    public:

        uint32_t GetDurationSecs() const { return m_durationSecs; }

        uint32_t GetDistanceMeters() const { return m_distanceMeters; }

        /// <summary>
        /// Initializes a new instance of the <see cref="RouteLegFromGoogleApi"/> class.
        /// </summary>
        RouteLegFromGoogleApi()
            : m_startPosition{ 0.0, 0.0, 0.0 }
            , m_endPosition{ 0.0, 0.0, 0.0 }
            , m_durationSecs(0)
            , m_distanceMeters(0)
        {}

        virtual ~RouteLegFromGoogleApi() {}

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
                    m_distanceMeters = node.at(L"value").as_integer();

                node = jsonLegObj[L"duration"];

                if (!node.is_null())
                    m_durationSecs = node.at(L"value").as_integer();

                node = jsonLegObj.at(L"start_location");
                m_startPosition.Latitude = node.at(L"lat").as_double();
                m_startPosition.Longitude = node.at(L"lng").as_double();

                node = jsonLegObj.at(L"end_location");
                m_endPosition.Latitude = node.at(L"lat").as_double();
                m_endPosition.Longitude = node.at(L"lng").as_double();

                web::json::array steps = jsonLegObj.at(L"steps").as_array();

                m_manuevers.reserve(steps.size());

                for (auto &entry : steps)
                {
                    std::unique_ptr<IRouteLegManeuverFWApi> maneuver(new RouteLegManeuverFromGoogleApi());
                    maneuver->ParseFromJSON(entry);
                    m_manuevers.push_back(std::move(maneuver));
                }
            }
            catch (core::IAppException &)
            {
                throw; // just forward exceptions for errors that have already been handled
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
            catch (Platform::Exception ^ex)
            {
                std::wstring_convert<std::codecvt_utf8<wchar_t>> transcoder;
                std::ostringstream oss;
                oss << "Generic failure when parsing leg JSON response from Google Maps Directions API: "
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
            geoPath.push_back(m_startPosition);

            for (auto &manuever : m_manuevers)
                manuever->AppendToPath(geoPath);

            geoPath.push_back(m_endPosition);
        }
    };


    /// <summary>
    /// Holds data obtained by parsing the JSON
    /// response from Google Maps Directions API.
    /// </summary>
    class RouteFromGoogleApi : public IRouteFromWebAPI
    {
    public:

        virtual ~RouteFromGoogleApi() {}

        /// <summary>
        /// Parses JSON for a route returned by Google Maps Directions API.
        /// </summary>
        /// <param name="jsonRouteObj">The JSON for a route.</param>
        virtual void ParseFromJSON(web::json::value &jsonRouteObj) override
        {
            CALL_STACK_TRACE;

            try
            {
                uint32_t totalDurationSecs(0);
                uint32_t totalDistanceMeters(0);

                web::json::array legs = jsonRouteObj.at(L"legs").as_array();

                m_legs.reserve(legs.size());

                for (auto &entry : legs)
                {
                    std::unique_ptr<RouteLegFromGoogleApi> leg(new RouteLegFromGoogleApi());
                    leg->ParseFromJSON(entry);

                    totalDistanceMeters += leg->GetDistanceMeters();
                    totalDurationSecs += leg->GetDurationSecs();

                    m_legs.push_back(std::move(leg));
                }

                std::wostringstream woss;
                woss << ToTimeSpanText(totalDurationSecs) << L" (" << ToDistanceText(totalDistanceMeters) << L")";

                m_mainInfo = ref new Platform::String(woss.str().c_str());
                
                woss.str(L"");

                auto fare = jsonRouteObj[L"fare"];

                if (!fare.is_null())
                    woss << L"Fare: " << fare.at(L"text").as_string() << std::endl;

                web::json::array warnings = jsonRouteObj.at(L"warnings").as_array();

                for (auto &entry : warnings)
                    woss << L"Warning! " << entry.as_string() << std::endl;

                woss << L"Copyrights: " << jsonRouteObj.at(L"copyrights").as_string();

                m_moreInfo = ref new Platform::String(woss.str().c_str());
            }
            catch (core::IAppException &)
            {
                throw; // just forward exceptions for errors that have already been handled
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
            catch (Platform::Exception ^ex)
            {
                std::wstring_convert<std::codecvt_utf8<wchar_t>> transcoder;
                std::ostringstream oss;
                oss << "Generic failure when parsing route JSON response from Google Maps Directions API: "
                    << transcoder.to_bytes(ex->Message->Data());

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
        _ASSERTE(waypoints->Size > 2);

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
    /// Get the best result for the provided waypoints and requesting to Google Maps API.
    /// </summary>
    /// <param name="waypoints">The required waypoints.</param>
    /// <param name="requiredTime">When not null, is the required departure OR arrival time.</param>
    /// <param name="restrictions">The restrictions to apply in the result.</param>
    /// <returns>The routes returned from Google Maps Directions API.</returns>
    task<RoutesFromWebApi> GetRoutesFromGoogleAsync(Collections::IVectorView<Waypoint ^> ^waypoints,
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
            utils::SerializeTo(buffer, destination.Latitude, L",", destination.Longitude);
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
            if (waypoints->Size > 2)
                uri->append_query(L"waypoints", ConvertMidWayptsToGoogleFormat(waypoints));

            // Constants:
            uri->append_query(L"mode",         L"driving");
            uri->append_query(L"alternatives", L"true");
            uri->append_query(L"units",        L"metric");

            std::string key = core::AppConfig::GetSettings().application.GetString("googleApiKey", "NOT SET");

            uri->append_query(L"key", utility::conversions::to_utf16string(key));
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
                throw core::AppException<std::runtime_error>(
                    "HTTP request to Google Maps Directions API has failed!",
                    utility::conversions::to_utf8string(response.reason_phrase())
                );
            }

            return response.extract_json();

        }, task_continuation_context::use_arbitrary())
        .then([](web::json::value &jsonBody)
        {
            CALL_STACK_TRACE;

            /////////////////////////////////////
            // Parse Google API JSON response

            try
            {
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

                RoutesFromWebApi result(new std::vector<std::unique_ptr<IRouteFromWebAPI>>());
                result->reserve(jsonRoutes.size());

                // parse the routes:
                for (auto &entry : jsonRoutes)
                {
                    std::unique_ptr<IRouteFromWebAPI> route(new RouteFromGoogleApi());
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
