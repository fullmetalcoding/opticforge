// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "project/ProjectSerializer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>


namespace opticforge::project
{
    namespace
    {

        using nlohmann::json;

        [[noreturn]] void fail(const std::string& message)
        {
            throw std::runtime_error(
                "OpticForge project deserialization failed: " + message);
        }

        json serializeVec2(const glm::vec2& value)
        {
            return json::array({
                value.x,
                value.y
                });
        }

        json serializeVec3(const glm::dvec3& value)
        {
            return json::array({
                value.x,
                value.y,
                value.z
                });
        }

        glm::vec2 deserializeVec2(const json& value)
        {
            return glm::vec2(
                value.at(0).get<float>(),
                value.at(1).get<float>());
        }

        glm::dvec3 deserializeVec3(const json& value)
        {
            return glm::dvec3(
                value.at(0).get<double>(),
                value.at(1).get<double>(),
                value.at(2).get<double>());
        }

        json serializeTransform(
            const optics::Transform& transform)
        {
            const glm::dquat& rotation =
                transform.rotation();

            return {
                {
                    "position",
                    serializeVec3(transform.position())
                },
                {
                    "rotation",
                    {
                        { "w", rotation.w },
                        { "x", rotation.x },
                        { "y", rotation.y },
                        { "z", rotation.z }
                    }
                }
            };
        }

        optics::Transform deserializeTransform(
            const json& value)
        {
            const glm::dvec3 position =
                deserializeVec3(
                    value.at("position"));

            const json& rotationJson =
                value.at("rotation");

            const double w =
                rotationJson.at("w").get<double>();

            const double x =
                rotationJson.at("x").get<double>();

            const double y =
                rotationJson.at("y").get<double>();

            const double z =
                rotationJson.at("z").get<double>();

            const double normSquared =
                w * w +
                x * x +
                y * y +
                z * z;

            if (!std::isfinite(normSquared) ||
                normSquared <=
                std::numeric_limits<double>::epsilon())
            {
                fail(
                    "transform contains a zero or non-finite quaternion.");
            }

            return optics::Transform(
                position,
                glm::dquat(
                    w,
                    x,
                    y,
                    z));
        }

        json serializeSurfaceGeometry(
            const optics::SurfaceGeometry& geometry)
        {
            return std::visit(
                [](const auto& concreteGeometry) -> json
                {
                    using Geometry =
                        std::decay_t<
                        decltype(concreteGeometry)>;

                    if constexpr (
                        std::is_same_v<
                        Geometry,
                        optics::PlaneGeometry>)
                    {
                        return {
                            { "type", "plane" }
                        };
                    }
                    else if constexpr (
                        std::is_same_v<
                        Geometry,
                        optics::ConicGeometry>)
                    {
                        return {
                            { "type", "conic" },
                            {
                                "radiusOfCurvature",
                                concreteGeometry.radiusOfCurvature()
                            },
                            {
                                "conicConstant",
                                concreteGeometry.conicConstant()
                            }
                        };
                    }
                    else
                    {
                        static_assert(
                            !sizeof(Geometry),
                            "Unhandled SurfaceGeometry variant.");
                    }
                },
                geometry);
        }

        optics::SurfaceGeometry deserializeSurfaceGeometry(
            const json& value)
        {
            const std::string type =
                value.at("type").get<std::string>();

            if (type == "plane")
            {
                return optics::PlaneGeometry{};
            }

            if (type == "conic")
            {
                return optics::ConicGeometry{
                    value.at(
                        "radiusOfCurvature").get<double>(),
                    value.at(
                        "conicConstant").get<double>()
                };
            }

            fail(
                "unknown optical surface geometry type '" +
                type +
                "'.");
        }

        json serializeAperture(
            const optics::Aperture& aperture)
        {
            return std::visit(
                [](const auto& concreteAperture) -> json
                {
                    using Aperture =
                        std::decay_t<
                        decltype(concreteAperture)>;

                    if constexpr (
                        std::is_same_v<
                        Aperture,
                        optics::CircularAperture>)
                    {
                        return {
                            { "type", "circular" },
                            { "radius", concreteAperture.radius }
                        };
                    }
                    else if constexpr (
                        std::is_same_v<
                        Aperture,
                        optics::AnnularAperture>)
                    {
                        return {
                            { "type", "annular" },
                            {
                                "innerRadius",
                                concreteAperture.innerRadius
                            },
                            {
                                "outerRadius",
                                concreteAperture.outerRadius
                            }
                        };
                    }
                    else if constexpr (
                        std::is_same_v<
                        Aperture,
                        optics::RectangularAperture>)
                    {
                        return {
                            { "type", "rectangular" },
                            { "width", concreteAperture.width },
                            { "height", concreteAperture.height }
                        };
                    }
                    else if constexpr (
                        std::is_same_v<
                        Aperture,
                        optics::EllipticalAperture>)
                    {
                        return {
                            { "type", "elliptical" },
                            { "radiusX", concreteAperture.radiusX },
                            { "radiusY", concreteAperture.radiusY }
                        };
                    }
                    else
                    {
                        static_assert(
                            !sizeof(Aperture),
                            "Unhandled ApertureGeometry variant.");
                    }
                },
                aperture.geometry());
        }

        optics::Aperture deserializeAperture(
            const json& value)
        {
            const std::string type =
                value.at("type").get<std::string>();

            if (type == "circular")
            {
                return optics::Aperture{
                    optics::CircularAperture{
                        value.at("radius").get<double>()
                    }
                };
            }
            if (type == "elliptical")
            {
                return optics::Aperture{
                    optics::EllipticalAperture{
                        value.at("radiusX").get<double>(),
                        value.at("radiusY").get<double>()
                    }
                };
            }

            if (type == "annular")
            {
                const double innerRadius =
                    value.at(
                        "innerRadius").get<double>();

                const double outerRadius =
                    value.at(
                        "outerRadius").get<double>();

                // AnnularAperture's constructor swaps these values when they
                // are reversed. Loading should never silently change file data.
                if (innerRadius > outerRadius)
                {
                    fail(
                        "annular aperture innerRadius exceeds outerRadius.");
                }

                return optics::Aperture{
                    optics::AnnularAperture{
                        innerRadius,
                        outerRadius
                    }
                };
            }

            if (type == "rectangular")
            {
                return optics::Aperture{
                    optics::RectangularAperture{
                        value.at("width").get<double>(),
                        value.at("height").get<double>()
                    }
                };
            }

            fail(
                "unknown aperture type '" +
                type +
                "'.");
        }

        json serializeOpticalInterface(
            const optics::OpticalInterface& opticalInterface)
        {
            return std::visit(
                [](const auto& concreteInterface) -> json
                {
                    using Interface =
                        std::decay_t<
                        decltype(concreteInterface)>;

                    if constexpr (
                        std::is_same_v<
                        Interface,
                        optics::RefractiveInterface>)
                    {
                        return {
                            { "type", "refractive" },
                            {
                                "negativeSideMaterial",
                                concreteInterface.negativeSideMaterial
                            },
                            {
                                "positiveSideMaterial",
                                concreteInterface.positiveSideMaterial
                            }
                        };
                    }
                    else if constexpr (
                        std::is_same_v<
                        Interface,
                        optics::ReflectiveInterface>)
                    {
                        return {
                            { "type", "reflective" },
                            {
                                "reflectivity",
                                concreteInterface.reflectivity
                            }
                        };
                    }
                    else if constexpr (
                        std::is_same_v<
                        Interface,
                        optics::DetectorInterface>)
                    {
                        return {
                            { "type", "detector" }
                        };
                    }
                    else if constexpr (
                        std::is_same_v<
                        Interface,
                        optics::AbsorbingInterface>)
                    {
                        return {
                            { "type", "absorbing" }
                        };
                    }
                    else
                    {
                        static_assert(
                            !sizeof(Interface),
                            "Unhandled OpticalInterfaceType variant.");
                    }
                },
                opticalInterface.type());
        }

        optics::OpticalInterface deserializeOpticalInterface(
            const json& value)
        {
            const std::string type =
                value.at("type").get<std::string>();

            if (type == "refractive")
            {
                return optics::OpticalInterface{
                    optics::RefractiveInterface{
                        value.at(
                            "negativeSideMaterial").get<double>(),
                        value.at(
                            "positiveSideMaterial").get<double>()
                    }
                };
            }

            if (type == "reflective")
            {
                return optics::OpticalInterface{
                    optics::ReflectiveInterface{
                        value.at(
                            "reflectivity").get<double>()
                    }
                };
            }

            if (type == "detector")
            {
                return optics::OpticalInterface{
                    optics::DetectorInterface{}
                };
            }

            if (type == "absorbing")
            {
                return optics::OpticalInterface{
                    optics::AbsorbingInterface{}
                };
            }

            fail(
                "unknown optical interface type '" +
                type +
                "'.");
        }

        json serializeOpticalSurface(
            const optics::OpticalSurface& surface)
        {
            return {
                {
                    "transform",
                    serializeTransform(
                        surface.transform())
                },
                {
                    "geometry",
                    serializeSurfaceGeometry(
                        surface.geometry())
                },
                {
                    "aperture",
                    serializeAperture(
                        surface.aperture())
                },
                {
                    "interface",
                    serializeOpticalInterface(
                        surface.opticalInterface())
                }
            };
        }

        optics::OpticalSurface deserializeOpticalSurface(
            const json& value)
        {
            return optics::OpticalSurface{
                deserializeTransform(
                    value.at("transform")),
                deserializeSurfaceGeometry(
                    value.at("geometry")),
                deserializeAperture(
                    value.at("aperture")),
                deserializeOpticalInterface(
                    value.at("interface"))
            };
        }

        json serializePrimitive(
            const telescope::PrimitiveRecord& record)
        {
            json result =
                std::visit(
                    [](const auto& primitive) -> json
                    {
                        using Primitive =
                            std::decay_t<
                            decltype(primitive)>;

                        if constexpr (
                            std::is_same_v<
                            Primitive,
                            telescope::Lens>)
                        {
                            return {
                                { "type", "lens" },
                                {
                                    "transform",
                                    serializeTransform(
                                        primitive.transform)
                                },
                                {
                                    "frontSurface",
                                    serializeOpticalSurface(
                                        primitive.frontSurface)
                                },
                                {
                                    "rearSurface",
                                    serializeOpticalSurface(
                                        primitive.rearSurface)
                                },
                                {
                                    "centerThickness",
                                    primitive.centerThickness
                                }
                            };
                        }
                        else if constexpr (
                            std::is_same_v<
                            Primitive,
                            telescope::Mirror>)
                        {
                            return {
                                { "type", "mirror" },
                                {
                                    "transform",
                                    serializeTransform(
                                        primitive.transform)
                                },
                                {
                                    "surface",
                                    serializeOpticalSurface(
                                        primitive.surface)
                                },
                                {
                                    "thickness",
                                    primitive.thickness
                                },
                                {
                                    "centralHole",
                                    primitive.centralHole
                                }
                            };
                        }
                        else if constexpr (
                            std::is_same_v<
                            Primitive,
                            telescope::Detector>)
                        {
                            return {
                                { "type", "detector" },
                                {
                                    "transform",
                                    serializeTransform(
                                        primitive.transform)
                                },
                                {
                                    "surface",
                                    serializeOpticalSurface(
                                        primitive.surface)
                                },
                                {
                                    "widthPixels",
                                    primitive.widthPixels
                                },
                                {
                                    "heightPixels",
                                    primitive.heightPixels
                                },
                                {
                                    "pixelPitchUm",
                                    primitive.pixelPitchUm
                                }
                            };
                        }
                        else
                        {
                            static_assert(
                                !sizeof(Primitive),
                                "Unhandled TelescopePrimitive variant.");
                        }
                    },
                    record.primitive);

            result["id"] = record.id;
            result["name"] = record.name; 

            return result;
        }

        telescope::PrimitiveRecord deserializePrimitive(
            const json& value)
        {
            const telescope::PrimitiveId id =
                value.at("id").get<
                telescope::PrimitiveId>();

            if (id == 0)
            {
                fail(
                    "primitive id 0 is reserved.");
            }


            const std::string type =
                value.at("type").get<std::string>();

            std::string name = "Unnamed " + type;

            if (value.contains("name")) {
                name = value.at("name");
            }

            if (type == "lens")
            {
                telescope::Lens lens;

                lens.transform =
                    deserializeTransform(
                        value.at("transform"));

                lens.frontSurface =
                    deserializeOpticalSurface(
                        value.at("frontSurface"));

                lens.rearSurface =
                    deserializeOpticalSurface(
                        value.at("rearSurface"));

                lens.centerThickness =
                    value.at(
                        "centerThickness").get<double>();

                return {
                    id,
                    std::move(lens),
                    name
                };
            }

            if (type == "mirror")
            {
                telescope::Mirror mirror;

                mirror.transform =
                    deserializeTransform(
                        value.at("transform"));

                mirror.surface =
                    deserializeOpticalSurface(
                        value.at("surface"));

                mirror.thickness =
                    value.at(
                        "thickness").get<double>();

                mirror.centralHole =
                    value.at(
                        "centralHole").get<double>();

                return {
                    id,
                    std::move(mirror),
                    name
                };
            }

            if (type == "detector")
            {
                telescope::Detector detector;

                detector.transform =
                    deserializeTransform(
                        value.at("transform"));

                detector.surface =
                    deserializeOpticalSurface(
                        value.at("surface"));

                detector.widthPixels =
                    value.at(
                        "widthPixels").get<int>();

                detector.heightPixels =
                    value.at(
                        "heightPixels").get<int>();

                detector.pixelPitchUm =
                    value.at(
                        "pixelPitchUm").get<double>();
           
                return {
                    id,
                    std::move(detector),
                    name
                };
            }

            fail(
                "unknown telescope primitive type '" +
                type +
                "'.");

            
        }

        json serializeLaunchPupil(
            const telescope::LaunchPupil& pupil)
        {
            return {
                {
                    "transform",
                    serializeTransform(
                        pupil.transform)
                },
                {
                    "aperture",
                    serializeAperture(
                        pupil.aperture)
                },
                {
                    "localDirection",
                    serializeVec3(
                        pupil.localDirection)
                }
            };
        }

        telescope::LaunchPupil deserializeLaunchPupil(
            const json& value)
        {
            telescope::LaunchPupil pupil;

            pupil.transform =
                deserializeTransform(
                    value.at("transform"));

            pupil.aperture =
                deserializeAperture(
                    value.at("aperture"));

            pupil.localDirection =
                deserializeVec3(
                    value.at("localDirection"));

            const double directionLengthSquared =
                glm::dot(
                    pupil.localDirection,
                    pupil.localDirection);

            if (!std::isfinite(directionLengthSquared) ||
                directionLengthSquared <=
                std::numeric_limits<double>::epsilon())
            {
                fail(
                    "launch pupil localDirection must be non-zero.");
            }

            return pupil;
        }

        json serializeObservationPlane(
            const telescope::ObservationPlane& plane)
        {
            return {
                {
                    "transform",
                    serializeTransform(
                        plane.transform)
                },
                {
                    "surface",
                    serializeOpticalSurface(
                        plane.surface)
                },
                {
                    "displaySize",
                    serializeVec2(
                        plane.displaySize)
                },
                {
                    "infiniteExtent",
                    plane.infiniteExtent
                },
                {
                    "positiveCrossingOnly",
                    plane.positiveCrossingOnly
                }
            };
        }

        telescope::ObservationPlane
            deserializeObservationPlane(
                const json& value)
        {
            telescope::ObservationPlane plane;

            plane.transform =
                deserializeTransform(
                    value.at("transform"));

            plane.surface =
                deserializeOpticalSurface(
                    value.at("surface"));

            plane.displaySize =
                deserializeVec2(
                    value.at("displaySize"));

            plane.infiniteExtent =
                value.at(
                    "infiniteExtent").get<bool>();

            plane.positiveCrossingOnly =
                value.at(
                    "positiveCrossingOnly").get<bool>();

            return plane;
        }

    } // namespace

    nlohmann::json ProjectSerializer::serialize(
        const telescope::TelescopeProject& project)
    {
        json primitives =
            json::array();

        primitives.get_ref<
            json::array_t&>().reserve(
                project.primitives().size());

        std::unordered_set<
            telescope::PrimitiveId> ids;

        for (const auto& record :
            project.primitives())
        {
            if (record.id == 0)
            {
                throw std::runtime_error(
                    "Cannot serialize an OpticForge project "
                    "containing primitive id 0.");
            }

            if (!ids.insert(record.id).second)
            {
                throw std::runtime_error(
                    "Cannot serialize an OpticForge project "
                    "containing duplicate primitive id " +
                    std::to_string(record.id) +
                    ".");
            }

            primitives.push_back(
                serializePrimitive(record));
        }

        return {
            {
                "format",
                std::string(FormatName)
            },
            {
                "version",
                FormatVersion
            },
            {
                "project",
                {
                    {
                        "launchPupil",
                        serializeLaunchPupil(
                            project.getLaunchPupil())
                    },
                    {
                        "observationPlane",
                        serializeObservationPlane(
                            project.getObservationPlane())
                    },
                    {
                        "primitives",
                        std::move(primitives)
                    }
                }
            }
        };
    }

    telescope::TelescopeProject ProjectSerializer::deserialize(
        const nlohmann::json& document)
    {
        const std::string format =
            document.at(
                "format").get<std::string>();

        if (format != std::string(FormatName))
        {
            fail(
                "document format is '" +
                format +
                "', expected '" +
                std::string(FormatName) +
                "'.");
        }

        const int version =
            document.at(
                "version").get<int>();

        if (version != FormatVersion)
        {
            fail(
                "unsupported project format version " +
                std::to_string(version) +
                ".");
        }

        const json& projectJson =
            document.at("project");

        telescope::TelescopeProject project;

        // ProjectSerializer is a friend of TelescopeProject. This lets loading
        // restore stable IDs and the atomic project objects without exposing
        // persistence-only mutation through TelescopeProject's public API.
        project.m_primitives.clear();

        project.m_launchPupil =
            deserializeLaunchPupil(
                projectJson.at(
                    "launchPupil"));

        project.m_observationPlane =
            deserializeObservationPlane(
                projectJson.at(
                    "observationPlane"));

        // Primitive ID 0 is reserved for the observation plane. TelescopeProject
        // should initialize/reset m_nextPrimitive to 0, and addPrimitive()
        // pre-increments it, so ordinary primitive IDs begin at 1.
        project.m_nextPrimitive = 0;

        std::unordered_set<
            telescope::PrimitiveId> ids;

        for (const json& primitiveJson :
            projectJson.at("primitives"))
        {
            telescope::PrimitiveRecord record =
                deserializePrimitive(
                    primitiveJson);

            if (!ids.insert(record.id).second)
            {
                fail(
                    "duplicate primitive id " +
                    std::to_string(record.id) +
                    ".");
            }

            project.m_nextPrimitive =
                std::max(
                    project.m_nextPrimitive,
                    record.id);

            project.m_primitives.push_back(
                std::move(record));
        }

        return project;
    }

} // namespace opticforge::project
