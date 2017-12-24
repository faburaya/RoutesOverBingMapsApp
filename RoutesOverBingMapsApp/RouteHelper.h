#pragma once

#include "AppViewModel.h"
#include <3FD\preprocessing.h>
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
    /// Holds the result for a web request for routes.
    /// </summary>
    struct ListOfRoutes
    {
        IVector<Geopath ^> ^routes;
        GeoboundingBox ^boundaries;
    };


    task<IVector<MapRoute ^> ^> GetRoutesFromMicrosoftAsync(Collections::IVectorView<Waypoint ^> ^waypoints,
                                                            MapRouteOptimization optimization,
                                                            uint8 restrictions);

    task<ListOfRoutes> GetRoutesFromGoogleAsync(Collections::IVectorView<Waypoint ^> ^waypoints,
                                                TimeRequirement *requiredTime,
                                                uint8 restrictions);


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
    protected:

        std::vector<std::unique_ptr<IRouteLegManeuverFWApi>> m_manuevers;

        BasicGeoposition m_startPosition;
        BasicGeoposition m_endPosition;
        uint32_t m_durationSecs;
        uint32_t m_distanceMeters;

    public:

        /// <summary>
        /// Initializes a new instance of the <see cref="IRouteLegFWApi"/> class.
        /// </summary>
        IRouteLegFWApi()
            : m_startPosition{ 0.0, 0.0, 0.0 }
            , m_endPosition{ 0.0, 0.0, 0.0 }
            , m_durationSecs(0)
            , m_distanceMeters(0)
        {}

        /// <summary>
        /// Finalizes an instance of the <see cref="IRouteLegFWApi"/> class.
        /// </summary>
        virtual IRouteLegFWApi::~IRouteLegFWApi() {}

        virtual void ParseFromJSON(web::json::value &jsonLegObj) = 0;

        /// <summary>
        /// Appends the geographic positions that compose this route leg to the route path.
        /// </summary>
        /// <param name="geoPath">The vector to receive the geographic
        /// positions for composal of the route path.</param>
        void AppendToPath(std::vector<BasicGeoposition> &geoPath)
        {
            geoPath.push_back(m_startPosition);

            for (auto &manuever : m_manuevers)
                manuever->AppendToPath(geoPath);

            geoPath.push_back(m_endPosition);
        }
    };


    /// <summary>
    /// Interface for a route obtained by parsing JSON of a response
    /// from some Web API like Google Maps or Tomtom.
    /// </summary>
    class INTFOPT IRouteFromWebAPI
    {
    protected:

        std::vector<std::unique_ptr<IRouteLegFWApi>> m_legs;

        GeoboundingBox ^m_boundingBox;

    public:

        virtual void ParseFromJSON(web::json::value &jsonRouteObj) = 0;

        GeoboundingBox ^GetBounds() const { return m_boundingBox; }

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

}// end of namespace RoutesOverBingMapsApp