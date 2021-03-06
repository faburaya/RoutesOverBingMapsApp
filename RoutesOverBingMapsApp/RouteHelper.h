#pragma once

#include "AppViewModel.h"
#include <3FD\preprocessing.h>
#include <cpprest/json.h>
#include <memory>
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
    public enum class RouteRestriction
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


    /// <summary>
    /// Interface to access data about a manuever inside a route leg.
    /// </summary>
    class INTFOPT IRouteLegManeuverFWApi
    {
    public:

        /// <summary>
        /// Finalizes an instance of the <see cref="IRouteLegManeuverFWApi"/> class.
        /// </summary>
        virtual IRouteLegManeuverFWApi::~IRouteLegManeuverFWApi() {}

        virtual void ParseFromJSON(web::json::value &jsonManueverObj) = 0;

        virtual void AppendToPath(std::vector<BasicGeoposition> &geoPath) = 0;
    };


    /// <summary>
    /// Interface to access data about a route leg.
    /// </summary>
    class INTFOPT IRouteLegFWApi
    {
    public:

        /// <summary>
        /// Finalizes an instance of the <see cref="IRouteLegFWApi"/> class.
        /// </summary>
        virtual IRouteLegFWApi::~IRouteLegFWApi() {}

        virtual void ParseFromJSON(web::json::value &jsonLegObj) = 0;

        virtual void AppendToPath(std::vector<BasicGeoposition> &geoPath) const = 0;
    };


    /// <summary>
    /// Interface for a route obtained by parsing JSON of a response
    /// from some Web API like Google Maps or Tomtom.
    /// </summary>
    class INTFOPT IRouteFromWebAPI
    {
    protected:

        std::vector<std::unique_ptr<IRouteLegFWApi>> m_legs;

        Platform::String ^m_mainInfo;
        Platform::String ^m_moreInfo;

    public:

        virtual IRouteFromWebAPI::~IRouteFromWebAPI() {}

        virtual void ParseFromJSON(web::json::value &jsonRouteObj) = 0;

        Platform::String ^GetMainInfo() const { return m_mainInfo; }

        Platform::String ^GetMoreInfo() const { return m_moreInfo; }

        /// <summary>
        /// Generates the path.
        /// </summary>
        /// <param name="geoPath">The geo path.</param>
        void GeneratePath(std::vector<BasicGeoposition> &geoPath)
        {
            for (auto &leg : m_legs)
                leg->AppendToPath(geoPath);
        }
    };


    typedef std::shared_ptr<std::vector<std::unique_ptr<IRouteFromWebAPI>>> RoutesFromWebApi;


    task<IVector<MapRoute ^> ^> GetRoutesFromMicrosoftAsync(Collections::IVectorView<Waypoint ^> ^waypoints,
                                                            MapRouteOptimization optimization,
                                                            uint8 restrictions);

    task<RoutesFromWebApi> GetRoutesFromGoogleAsync(Collections::IVectorView<Waypoint ^> ^waypoints,
                                                    TimeRequirement *requiredTime,
                                                    uint8 restrictions);

    task<RoutesFromWebApi> GetRoutesFromTomTomAsync(Collections::IVectorView<Waypoint ^> ^waypoints,
                                                    TimeRequirement *requiredTime,
                                                    uint8 restrictions);

}// end of namespace RoutesOverBingMapsApp