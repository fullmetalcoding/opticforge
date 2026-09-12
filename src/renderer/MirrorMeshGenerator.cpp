#include "MirrorMeshGenerator.h"

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <type_traits>

#include "optics/Aperture.h"
#include "optics/ConicGeometry.h"
#include "optics/PlaneGeometry.h"

namespace opticforge
{

    namespace
    {

        struct MirrorApertureDimensions
        {
            double innerRadius = 0.0;
            double outerRadius = 0.0;
        };


        double surfaceSag(
            const optics::SurfaceGeometry& geometry,
            double radius)
        {
            return std::visit(
                [radius](const auto& g) -> double
                {
                    using T =
                        std::decay_t<decltype(g)>;

                    if constexpr (
                        std::is_same_v<
                        T,
                        optics::PlaneGeometry>)
                    {
                        return 0.0;
                    }
                    else
                    {
                        return g.sag(radius);
                    }
                },
                geometry);
        }


        glm::dvec3 surfaceNormal(
            const optics::SurfaceGeometry& geometry,
            const glm::dvec3& point)
        {
            return std::visit(
                [&](const auto& g)
                {
                    return g.normalAt(point);
                },
                geometry);
        }


        MirrorApertureDimensions mirrorApertureDimensions(
            const optics::Aperture& aperture)
        {
            return std::visit(
                [](const auto& a)
                -> MirrorApertureDimensions
                {
                    using T =
                        std::decay_t<decltype(a)>;

                    if constexpr (
                        std::is_same_v<
                        T,
                        optics::CircularAperture>)
                    {
                        return {
                            0.0,
                            a.radius
                        };
                    }
                    else if constexpr (
                        std::is_same_v<
                        T,
                        optics::AnnularAperture>)
                    {
                        return {
                            a.innerRadius,
                            a.outerRadius
                        };
                    }
                    else
                    {
                        throw std::runtime_error(
                            "MirrorMeshGenerator requires a "
                            "circular or annular aperture.");
                    }
                },
                aperture.geometry());
        }


        glm::vec3 toFloat(
            const glm::dvec3& v)
        {
            return glm::vec3(
                static_cast<float>(v.x),
                static_cast<float>(v.y),
                static_cast<float>(v.z));
        }

    } // anonymous namespace


    MeshData MirrorMeshGenerator::generate(
        const optics::OpticalSurface& opticalSurface,
        double thickness,
        std::uint32_t radialSegments,
        std::uint32_t angularSegments)
    {
        if (thickness <= 0.0)
        {
            throw std::invalid_argument(
                "Mirror thickness must be greater than zero.");
        }

        if (radialSegments < 1)
        {
            throw std::invalid_argument(
                "radialSegments must be >= 1.");
        }

        if (angularSegments < 3)
        {
            throw std::invalid_argument(
                "angularSegments must be >= 3.");
        }

        const MirrorApertureDimensions aperture =
            mirrorApertureDimensions(
                opticalSurface.aperture());

        const double innerRadius =
            aperture.innerRadius;

        const double outerRadius =
            aperture.outerRadius;

        if (outerRadius <= 0.0)
        {
            throw std::invalid_argument(
                "Mirror outer radius must be greater than zero.");
        }

        if (innerRadius < 0.0)
        {
            throw std::invalid_argument(
                "Mirror inner radius cannot be negative.");
        }

        if (innerRadius >= outerRadius)
        {
            throw std::invalid_argument(
                "Mirror inner radius must be smaller than outer radius.");
        }

        const bool hasCentralHole =
            innerRadius > 0.0;

        MeshData mesh;

        //
        // -------------------------------------------------------------------------
        // Optical face
        // -------------------------------------------------------------------------
        //

        std::uint32_t frontCenter = 0;

        if (!hasCentralHole)
        {
            frontCenter =
                static_cast<std::uint32_t>(
                    mesh.vertices.size());

            glm::dvec3 centerNormal =
                surfaceNormal(
                    opticalSurface.geometry(),
                    glm::dvec3(
                        0.0,
                        0.0,
                        0.0));

            if (centerNormal.z > 0.0)
                centerNormal = -centerNormal;

            mesh.vertices.push_back(
                MeshVertex{
                    glm::vec3(
                        0.0f,
                        0.0f,
                        0.0f),
                    toFloat(centerNormal)
                });
        }

        const std::uint32_t frontRingStart =
            static_cast<std::uint32_t>(
                mesh.vertices.size());

        //
        // Solid mirrors begin at ring 1 because ring 0 would duplicate
        // the center vertex.
        //
        // Annular mirrors begin at ring 0 because that ring is the
        // inner edge of the hole.
        //
        const std::uint32_t firstRing =
            hasCentralHole
            ? 0
            : 1;

        for (std::uint32_t ring = firstRing;
            ring <= radialSegments;
            ++ring)
        {
            const double fraction =
                static_cast<double>(ring) /
                static_cast<double>(
                    radialSegments);

            const double ringRadius =
                innerRadius +
                (outerRadius - innerRadius) *
                fraction;

            const double z =
                surfaceSag(
                    opticalSurface.geometry(),
                    ringRadius);

            if (!std::isfinite(z))
            {
                throw std::runtime_error(
                    "Mirror aperture extends beyond "
                    "the valid optical surface domain.");
            }

            for (std::uint32_t segment = 0;
                segment < angularSegments;
                ++segment)
            {
                const double angle =
                    2.0 *
                    std::numbers::pi *
                    static_cast<double>(segment) /
                    static_cast<double>(
                        angularSegments);

                const double x =
                    ringRadius *
                    std::cos(angle);

                const double y =
                    ringRadius *
                    std::sin(angle);

                glm::dvec3 normal =
                    surfaceNormal(
                        opticalSurface.geometry(),
                        glm::dvec3(
                            x,
                            y,
                            z));

                //
                // Optical face outward normal points generally toward -Z.
                //
                if (normal.z > 0.0)
                    normal = -normal;

                mesh.vertices.push_back(
                    MeshVertex{
                        glm::vec3(
                            static_cast<float>(x),
                            static_cast<float>(y),
                            static_cast<float>(z)),
                        toFloat(normal)
                    });
            }
        }

        //
        // Solid mirror only: connect center to first radial ring.
        //
        if (!hasCentralHole)
        {
            for (std::uint32_t i = 0;
                i < angularSegments;
                ++i)
            {
                const std::uint32_t next =
                    (i + 1) %
                    angularSegments;

                mesh.indices.insert(
                    mesh.indices.end(),
                    {
                        frontCenter,
                        frontRingStart + next,
                        frontRingStart + i
                    });
            }
        }

        const std::uint32_t frontRingCount =
            hasCentralHole
            ? radialSegments + 1
            : radialSegments;

        //
        // Connect adjacent optical-face rings.
        //
        for (std::uint32_t ring = 0;
            ring + 1 < frontRingCount;
            ++ring)
        {
            const std::uint32_t innerStart =
                frontRingStart +
                ring *
                angularSegments;

            const std::uint32_t outerStart =
                frontRingStart +
                (ring + 1) *
                angularSegments;

            for (std::uint32_t i = 0;
                i < angularSegments;
                ++i)
            {
                const std::uint32_t next =
                    (i + 1) %
                    angularSegments;

                const std::uint32_t i0 =
                    innerStart + i;

                const std::uint32_t i1 =
                    innerStart + next;

                const std::uint32_t o0 =
                    outerStart + i;

                const std::uint32_t o1 =
                    outerStart + next;

                mesh.indices.insert(
                    mesh.indices.end(),
                    {
                        i0, o1, o0,
                        i0, i1, o1
                    });
            }
        }


        //
        // -------------------------------------------------------------------------
        // Flat rear face
        // -------------------------------------------------------------------------
        //

        std::uint32_t rearCenter = 0;

        if (!hasCentralHole)
        {
            rearCenter =
                static_cast<std::uint32_t>(
                    mesh.vertices.size());

            mesh.vertices.push_back(
                MeshVertex{
                    glm::vec3(
                        0.0f,
                        0.0f,
                        static_cast<float>(
                            thickness)),
                    glm::vec3(
                        0.0f,
                        0.0f,
                        1.0f)
                });
        }

        const std::uint32_t rearRingStart =
            static_cast<std::uint32_t>(
                mesh.vertices.size());

        for (std::uint32_t ring = firstRing;
            ring <= radialSegments;
            ++ring)
        {
            const double fraction =
                static_cast<double>(ring) /
                static_cast<double>(
                    radialSegments);

            const double ringRadius =
                innerRadius +
                (outerRadius - innerRadius) *
                fraction;

            for (std::uint32_t segment = 0;
                segment < angularSegments;
                ++segment)
            {
                const double angle =
                    2.0 *
                    std::numbers::pi *
                    static_cast<double>(segment) /
                    static_cast<double>(
                        angularSegments);

                const double x =
                    ringRadius *
                    std::cos(angle);

                const double y =
                    ringRadius *
                    std::sin(angle);

                mesh.vertices.push_back(
                    MeshVertex{
                        glm::vec3(
                            static_cast<float>(x),
                            static_cast<float>(y),
                            static_cast<float>(
                                thickness)),
                        glm::vec3(
                            0.0f,
                            0.0f,
                            1.0f)
                    });
            }
        }

        //
        // Solid mirror only: rear center fan.
        //
        if (!hasCentralHole)
        {
            for (std::uint32_t i = 0;
                i < angularSegments;
                ++i)
            {
                const std::uint32_t next =
                    (i + 1) %
                    angularSegments;

                mesh.indices.insert(
                    mesh.indices.end(),
                    {
                        rearCenter,
                        rearRingStart + i,
                        rearRingStart + next
                    });
            }
        }

        const std::uint32_t rearRingCount =
            hasCentralHole
            ? radialSegments + 1
            : radialSegments;

        //
        // Connect adjacent rear rings.
        //
        for (std::uint32_t ring = 0;
            ring + 1 < rearRingCount;
            ++ring)
        {
            const std::uint32_t innerStart =
                rearRingStart +
                ring *
                angularSegments;

            const std::uint32_t outerStart =
                rearRingStart +
                (ring + 1) *
                angularSegments;

            for (std::uint32_t i = 0;
                i < angularSegments;
                ++i)
            {
                const std::uint32_t next =
                    (i + 1) %
                    angularSegments;

                const std::uint32_t i0 =
                    innerStart + i;

                const std::uint32_t i1 =
                    innerStart + next;

                const std::uint32_t o0 =
                    outerStart + i;

                const std::uint32_t o1 =
                    outerStart + next;

                mesh.indices.insert(
                    mesh.indices.end(),
                    {
                        i0, o0, o1,
                        i0, o1, i1
                    });
            }
        }


        //
        // -------------------------------------------------------------------------
        // Outer cylindrical wall
        // -------------------------------------------------------------------------
        //

        const double frontOuterZ =
            surfaceSag(
                opticalSurface.geometry(),
                outerRadius);

        if (!std::isfinite(frontOuterZ))
        {
            throw std::runtime_error(
                "Mirror outer edge lies outside "
                "the valid surface geometry domain.");
        }

        if (thickness <= frontOuterZ)
        {
            throw std::runtime_error(
                "Mirror optical surface intersects or passes "
                "through the rear substrate surface.");
        }

        const std::uint32_t outerWallStart =
            static_cast<std::uint32_t>(
                mesh.vertices.size());

        for (std::uint32_t i = 0;
            i < angularSegments;
            ++i)
        {
            const double angle =
                2.0 *
                std::numbers::pi *
                static_cast<double>(i) /
                static_cast<double>(
                    angularSegments);

            const double c =
                std::cos(angle);

            const double s =
                std::sin(angle);

            const float x =
                static_cast<float>(
                    outerRadius * c);

            const float y =
                static_cast<float>(
                    outerRadius * s);

            const glm::vec3 normal(
                static_cast<float>(c),
                static_cast<float>(s),
                0.0f);

            mesh.vertices.push_back(
                MeshVertex{
                    glm::vec3(
                        x,
                        y,
                        static_cast<float>(
                            frontOuterZ)),
                    normal
                });

            mesh.vertices.push_back(
                MeshVertex{
                    glm::vec3(
                        x,
                        y,
                        static_cast<float>(
                            thickness)),
                    normal
                });
        }

        for (std::uint32_t i = 0;
            i < angularSegments;
            ++i)
        {
            const std::uint32_t next =
                (i + 1) %
                angularSegments;

            const std::uint32_t front0 =
                outerWallStart +
                i * 2;

            const std::uint32_t rear0 =
                front0 + 1;

            const std::uint32_t front1 =
                outerWallStart +
                next * 2;

            const std::uint32_t rear1 =
                front1 + 1;

            mesh.indices.insert(
                mesh.indices.end(),
                {
                    front0, rear1, rear0,
                    front0, front1, rear1
                });
        }


        //
        // -------------------------------------------------------------------------
        // Inner cylindrical wall
        // -------------------------------------------------------------------------
        //
        // Only present for annular mirrors.
        //

        if (hasCentralHole)
        {
            const double frontInnerZ =
                surfaceSag(
                    opticalSurface.geometry(),
                    innerRadius);

            if (!std::isfinite(frontInnerZ))
            {
                throw std::runtime_error(
                    "Mirror central hole lies outside "
                    "the valid surface geometry domain.");
            }

            if (thickness <= frontInnerZ)
            {
                throw std::runtime_error(
                    "Mirror optical surface intersects or passes "
                    "through the rear substrate at the central hole.");
            }

            const std::uint32_t innerWallStart =
                static_cast<std::uint32_t>(
                    mesh.vertices.size());

            for (std::uint32_t i = 0;
                i < angularSegments;
                ++i)
            {
                const double angle =
                    2.0 *
                    std::numbers::pi *
                    static_cast<double>(i) /
                    static_cast<double>(
                        angularSegments);

                const double c =
                    std::cos(angle);

                const double s =
                    std::sin(angle);

                const float x =
                    static_cast<float>(
                        innerRadius * c);

                const float y =
                    static_cast<float>(
                        innerRadius * s);

                //
                // Inner wall outward normal points toward the hole,
                // i.e. inward toward the optical axis.
                //
                const glm::vec3 normal(
                    static_cast<float>(-c),
                    static_cast<float>(-s),
                    0.0f);

                //
                // Optical-face edge of hole.
                //
                mesh.vertices.push_back(
                    MeshVertex{
                        glm::vec3(
                            x,
                            y,
                            static_cast<float>(
                                frontInnerZ)),
                        normal
                    });

                //
                // Rear-face edge of hole.
                //
                mesh.vertices.push_back(
                    MeshVertex{
                        glm::vec3(
                            x,
                            y,
                            static_cast<float>(
                                thickness)),
                        normal
                    });
            }

            for (std::uint32_t i = 0;
                i < angularSegments;
                ++i)
            {
                const std::uint32_t next =
                    (i + 1) %
                    angularSegments;

                const std::uint32_t front0 =
                    innerWallStart +
                    i * 2;

                const std::uint32_t rear0 =
                    front0 + 1;

                const std::uint32_t front1 =
                    innerWallStart +
                    next * 2;

                const std::uint32_t rear1 =
                    front1 + 1;

                //
                // Reverse winding relative to the outer wall because
                // this surface faces inward toward the hole.
                //
                mesh.indices.insert(
                    mesh.indices.end(),
                    {
                        front0, rear0, rear1,
                        front0, rear1, front1
                    });
            }
        }

        return mesh;
    }

} // namespace opticforge