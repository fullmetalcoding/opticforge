// LensMeshGenerator.cpp

#include "LensMeshGenerator.h"

#include <algorithm>
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

        struct LensApertureDimensions
        {
            double innerRadius = 0.0;
            double outerRadius = 0.0;
        };


        double surfaceSag(
            const optics::SurfaceGeometry& geometry,
            double r)
        {
            return std::visit(
                [r](const auto& g) -> double
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
                        return g.sag(r);
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


        LensApertureDimensions apertureDimensions(
            const optics::Aperture& aperture)
        {
            return std::visit(
                [](const auto& a)
                -> LensApertureDimensions
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
                            "LensMeshGenerator requires "
                            "CircularAperture or AnnularAperture.");
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


    MeshData LensMeshGenerator::generate(
        const optics::OpticalSurface& frontSurface,
        const optics::OpticalSurface& rearSurface,
        double thickness,
        std::uint32_t radialSegments,
        std::uint32_t angularSegments)
    {
        if (thickness <= 0.0)
        {
            throw std::invalid_argument(
                "Lens thickness must be greater than zero.");
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

        //
        // Determine usable radial range from both optical surfaces.
        //
        const LensApertureDimensions frontAperture =
            apertureDimensions(
                frontSurface.aperture());

        const LensApertureDimensions rearAperture =
            apertureDimensions(
                rearSurface.aperture());

        //
        // Use only the radial region common to both surfaces.
        //
        // For example:
        //
        // front: 20..100
        // rear:  25..100
        //
        // generated lens: 25..100
        //
        const double innerRadius =
            std::max(
                frontAperture.innerRadius,
                rearAperture.innerRadius);

        const double outerRadius =
            std::min(
                frontAperture.outerRadius,
                rearAperture.outerRadius);

        if (outerRadius <= 0.0)
        {
            throw std::invalid_argument(
                "Lens outer radius must be greater than zero.");
        }

        if (innerRadius < 0.0)
        {
            throw std::invalid_argument(
                "Lens inner radius cannot be negative.");
        }

        if (innerRadius >= outerRadius)
        {
            throw std::invalid_argument(
                "Lens inner radius must be smaller "
                "than outer radius.");
        }

        const bool hasCentralHole =
            innerRadius > 0.0;

        MeshData mesh;

        //
        // Reserve approximate storage.
        //
        const std::size_t ringsPerFace =
            hasCentralHole
            ? static_cast<std::size_t>(
                radialSegments + 1)
            : static_cast<std::size_t>(
                radialSegments);

        const std::size_t centerVertices =
            hasCentralHole
            ? 0
            : 2;

        const std::size_t wallVertices =
            static_cast<std::size_t>(
                angularSegments) *
            (hasCentralHole ? 4 : 2);

        mesh.vertices.reserve(
            centerVertices +
            ringsPerFace *
            angularSegments *
            2 +
            wallVertices);


        //
        // =========================================================================
        // Front face
        // =========================================================================
        //
        // Front vertex is at local z = 0.
        //

        std::uint32_t frontCenter = 0;

        if (!hasCentralHole)
        {
            frontCenter =
                static_cast<std::uint32_t>(
                    mesh.vertices.size());

            glm::dvec3 frontCenterNormal =
                surfaceNormal(
                    frontSurface.geometry(),
                    glm::dvec3(
                        0.0,
                        0.0,
                        0.0));

            //
            // Front outward direction should generally point toward -Z.
            //
            if (frontCenterNormal.z > 0.0)
                frontCenterNormal =
                -frontCenterNormal;

            mesh.vertices.push_back(
                MeshVertex{
                    glm::vec3(
                        0.0f,
                        0.0f,
                        0.0f),
                    toFloat(
                        frontCenterNormal)
                });
        }

        const std::uint32_t frontRingStart =
            static_cast<std::uint32_t>(
                mesh.vertices.size());

        //
        // Solid lens:
        //     first generated ring is ring 1.
        //
        // Annular lens:
        //     ring 0 is the exact inner-hole boundary.
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
                    frontSurface.geometry(),
                    ringRadius);

            if (!std::isfinite(z))
            {
                throw std::runtime_error(
                    "Front surface aperture exceeds "
                    "valid geometry domain.");
            }

            for (std::uint32_t segment = 0;
                segment < angularSegments;
                ++segment)
            {
                const double angle =
                    2.0 *
                    std::numbers::pi *
                    static_cast<double>(
                        segment) /
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
                        frontSurface.geometry(),
                        glm::dvec3(
                            x,
                            y,
                            z));

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
        // Solid lens only:
        // connect center vertex to first radial ring.
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
        // Connect all neighboring front-face rings.
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
        // =========================================================================
        // Rear face
        // =========================================================================
        //
        // Rear surface vertex is at local z = thickness.
        //

        std::uint32_t rearCenter = 0;

        if (!hasCentralHole)
        {
            rearCenter =
                static_cast<std::uint32_t>(
                    mesh.vertices.size());

            glm::dvec3 rearCenterNormal =
                surfaceNormal(
                    rearSurface.geometry(),
                    glm::dvec3(
                        0.0,
                        0.0,
                        0.0));

            //
            // Rear outward direction should generally point toward +Z.
            //
            if (rearCenterNormal.z < 0.0)
                rearCenterNormal =
                -rearCenterNormal;

            mesh.vertices.push_back(
                MeshVertex{
                    glm::vec3(
                        0.0f,
                        0.0f,
                        static_cast<float>(
                            thickness)),
                    toFloat(
                        rearCenterNormal)
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

            const double sag =
                surfaceSag(
                    rearSurface.geometry(),
                    ringRadius);

            if (!std::isfinite(sag))
            {
                throw std::runtime_error(
                    "Rear surface aperture exceeds "
                    "valid geometry domain.");
            }

            const double z =
                thickness + sag;

            for (std::uint32_t segment = 0;
                segment < angularSegments;
                ++segment)
            {
                const double angle =
                    2.0 *
                    std::numbers::pi *
                    static_cast<double>(
                        segment) /
                    static_cast<double>(
                        angularSegments);

                const double x =
                    ringRadius *
                    std::cos(angle);

                const double y =
                    ringRadius *
                    std::sin(angle);

                //
                // normalAt() expects coordinates relative
                // to the rear surface's own local vertex.
                //
                glm::dvec3 normal =
                    surfaceNormal(
                        rearSurface.geometry(),
                        glm::dvec3(
                            x,
                            y,
                            sag));

                if (normal.z < 0.0)
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
        // Solid lens only:
        // rear center fan.
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
        // Connect neighboring rear-face rings.
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
        // =========================================================================
        // Outer cylindrical edge
        // =========================================================================
        //

        const double frontOuterZ =
            surfaceSag(
                frontSurface.geometry(),
                outerRadius);

        const double rearOuterZ =
            thickness +
            surfaceSag(
                rearSurface.geometry(),
                outerRadius);

        if (!std::isfinite(frontOuterZ) ||
            !std::isfinite(rearOuterZ))
        {
            throw std::runtime_error(
                "Lens outer edge lies outside "
                "valid surface domain.");
        }

        if (rearOuterZ <= frontOuterZ)
        {
            throw std::runtime_error(
                "Front and rear lens surfaces intersect "
                "at the outer aperture.");
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

            //
            // Front outer edge.
            //
            mesh.vertices.push_back(
                MeshVertex{
                    glm::vec3(
                        x,
                        y,
                        static_cast<float>(
                            frontOuterZ)),
                    normal
                });

            //
            // Rear outer edge.
            //
            mesh.vertices.push_back(
                MeshVertex{
                    glm::vec3(
                        x,
                        y,
                        static_cast<float>(
                            rearOuterZ)),
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

            const std::uint32_t f0 =
                outerWallStart +
                i * 2;

            const std::uint32_t r0 =
                f0 + 1;

            const std::uint32_t f1 =
                outerWallStart +
                next * 2;

            const std::uint32_t r1 =
                f1 + 1;

            mesh.indices.insert(
                mesh.indices.end(),
                {
                    f0, r1, r0,
                    f0, f1, r1
                });
        }


        //
        // =========================================================================
        // Inner cylindrical wall
        // =========================================================================
        //
        // Only generated for an annular aperture.
        //

        if (hasCentralHole)
        {
            const double frontInnerZ =
                surfaceSag(
                    frontSurface.geometry(),
                    innerRadius);

            const double rearInnerZ =
                thickness +
                surfaceSag(
                    rearSurface.geometry(),
                    innerRadius);

            if (!std::isfinite(frontInnerZ) ||
                !std::isfinite(rearInnerZ))
            {
                throw std::runtime_error(
                    "Lens inner aperture lies outside "
                    "valid surface domain.");
            }

            if (rearInnerZ <= frontInnerZ)
            {
                throw std::runtime_error(
                    "Front and rear lens surfaces intersect "
                    "at the inner aperture.");
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
                // The outward normal of the INNER wall points
                // inward toward the optical axis / hole.
                //
                const glm::vec3 normal(
                    static_cast<float>(-c),
                    static_cast<float>(-s),
                    0.0f);

                //
                // Front edge of central opening.
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
                // Rear edge of central opening.
                //
                mesh.vertices.push_back(
                    MeshVertex{
                        glm::vec3(
                            x,
                            y,
                            static_cast<float>(
                                rearInnerZ)),
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

                const std::uint32_t f0 =
                    innerWallStart +
                    i * 2;

                const std::uint32_t r0 =
                    f0 + 1;

                const std::uint32_t f1 =
                    innerWallStart +
                    next * 2;

                const std::uint32_t r1 =
                    f1 + 1;

                //
                // Opposite winding from the outer wall because
                // this surface faces inward.
                //
                mesh.indices.insert(
                    mesh.indices.end(),
                    {
                        f0, r0, r1,
                        f0, r1, f1
                    });
            }
        }

        return mesh;
    }

} // namespace opticforge