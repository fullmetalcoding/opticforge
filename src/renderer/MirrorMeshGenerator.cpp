// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "MirrorMeshGenerator.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "optics/Aperture.h"
#include "optics/ConicGeometry.h"
#include "optics/PlaneGeometry.h"

namespace opticforge
{
    namespace
    {
        constexpr double Pi =
            3.141592653589793238462643383279502884;


        //
        // =========================================================================
        // Aperture type detection
        // =========================================================================
        //
        // These traits deliberately detect aperture capabilities by their
        // parameter names rather than referring directly to every concrete
        // aperture type.
        //
        // This means that after adding:
        //
        //     struct EllipticalAperture
        //     {
        //         double radiusX;
        //         double radiusY;
        //         ...
        //     };
        //
        // to ApertureGeometry, this file does not need another explicit
        // EllipticalAperture branch.
        //

        template<typename T, typename = void>
        struct HasRadius :
            std::false_type
        {
        };

        template<typename T>
        struct HasRadius<
            T,
            std::void_t<
            decltype(
                std::declval<const T&>().radius)>>
            : std::true_type
        {
        };


        template<typename T, typename = void>
        struct HasInnerOuterRadius :
            std::false_type
        {
        };

        template<typename T>
        struct HasInnerOuterRadius<
            T,
            std::void_t<
            decltype(
                std::declval<const T&>().innerRadius),
            decltype(
                std::declval<const T&>().outerRadius)>>
            : std::true_type
        {
        };


        template<typename T, typename = void>
        struct HasWidthHeight :
            std::false_type
        {
        };

        template<typename T>
        struct HasWidthHeight<
            T,
            std::void_t<
            decltype(
                std::declval<const T&>().width),
            decltype(
                std::declval<const T&>().height)>>
            : std::true_type
        {
        };


        template<typename T, typename = void>
        struct HasRadiusXY :
            std::false_type
        {
        };

        template<typename T>
        struct HasRadiusXY<
            T,
            std::void_t<
            decltype(
                std::declval<const T&>().radiusX),
            decltype(
                std::declval<const T&>().radiusY)>>
            : std::true_type
        {
        };


        template<typename>
        struct DependentFalse :
            std::false_type
        {
        };


        //
        // =========================================================================
        // Sampled aperture representation
        // =========================================================================
        //
        // The rest of the mesh generator operates entirely on this representation.
        //
        // outerBoundary:
        //     Counter-clockwise boundary of the substrate.
        //
        // innerBoundary:
        //     Optional central hole. It must contain the same number of samples as
        //     outerBoundary, with corresponding angular/perimeter locations.
        //
        // This naturally supports:
        //
        //     circle
        //     annulus
        //     ellipse
        //     rectangle
        //
        // and can later support other centered/star-shaped apertures simply by
        // teaching sampleAperture() how to produce these contours.
        //

        struct SampledAperture
        {
            std::vector<glm::dvec2> outerBoundary;
            std::vector<glm::dvec2> innerBoundary;

            bool hasHole() const noexcept
            {
                return !innerBoundary.empty();
            }

            std::size_t perimeterCount() const noexcept
            {
                return outerBoundary.size();
            }
        };


        //
        // =========================================================================
        // Basic optical surface helpers
        // =========================================================================
        //

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


        glm::vec3 toFloat(
            const glm::dvec3& v)
        {
            return glm::vec3(
                static_cast<float>(v.x),
                static_cast<float>(v.y),
                static_cast<float>(v.z));
        }


        double radialDistance(
            const glm::dvec2& p)
        {
            return std::hypot(
                p.x,
                p.y);
        }


        double surfaceZAt(
            const optics::SurfaceGeometry& geometry,
            const glm::dvec2& point,
            double thickness)
        {
            const double radius =
                radialDistance(point);

            const double z =
                surfaceSag(
                    geometry,
                    radius);

            if (!std::isfinite(z))
            {
                throw std::runtime_error(
                    "Mirror aperture extends beyond "
                    "the valid optical surface domain.");
            }

            //
            // The substrate convention is:
            //
            //     optical face = sag(x, y)
            //     rear face    = thickness
            //
            // They must remain strictly separated.
            //
            if (z >= thickness)
            {
                throw std::runtime_error(
                    "Mirror optical surface intersects or passes "
                    "through the rear substrate surface.");
            }

            return z;
        }


        //
        // =========================================================================
        // Boundary sampling
        // =========================================================================
        //

        std::vector<glm::dvec2> sampleEllipse(
            double radiusX,
            double radiusY,
            std::uint32_t segmentCount)
        {
            if (
                !std::isfinite(radiusX) ||
                !std::isfinite(radiusY) ||
                radiusX <= 0.0 ||
                radiusY <= 0.0)
            {
                throw std::invalid_argument(
                    "Ellipse radii must be finite and greater than zero.");
            }

            segmentCount =
                std::max<std::uint32_t>(
                    segmentCount,
                    3);

            std::vector<glm::dvec2> points;

            points.reserve(
                segmentCount);

            for (std::uint32_t i = 0;
                i < segmentCount;
                ++i)
            {
                const double angle =
                    2.0 *
                    Pi *
                    static_cast<double>(i) /
                    static_cast<double>(
                        segmentCount);

                points.emplace_back(
                    radiusX * std::cos(angle),
                    radiusY * std::sin(angle));
            }

            return points;
        }


        std::vector<glm::dvec2> sampleRectangle(
            double width,
            double height,
            std::uint32_t requestedSegments)
        {
            if (
                !std::isfinite(width) ||
                !std::isfinite(height) ||
                width <= 0.0 ||
                height <= 0.0)
            {
                throw std::invalid_argument(
                    "Rectangle width and height must be finite "
                    "and greater than zero.");
            }

            //
            // Make every corner an explicit boundary sample.
            //
            // requestedSegments=96 therefore becomes 24 samples/edge.
            //
            // If requestedSegments is not divisible by four, round upward.
            //
            const std::uint32_t segmentsPerEdge =
                std::max<std::uint32_t>(
                    1,
                    (requestedSegments + 3) / 4);

            const double halfWidth =
                width * 0.5;

            const double halfHeight =
                height * 0.5;

            std::vector<glm::dvec2> points;

            points.reserve(
                static_cast<std::size_t>(
                    segmentsPerEdge) *
                4);

            //
            // Walk counter-clockwise.
            //
            // Each corner belongs to exactly one edge so there are no duplicate
            // perimeter vertices.
            //

            // Bottom: bottom-left -> bottom-right
            for (std::uint32_t i = 0;
                i < segmentsPerEdge;
                ++i)
            {
                const double t =
                    static_cast<double>(i) /
                    static_cast<double>(
                        segmentsPerEdge);

                points.emplace_back(
                    -halfWidth +
                    width * t,
                    -halfHeight);
            }

            // Right: bottom-right -> top-right
            for (std::uint32_t i = 0;
                i < segmentsPerEdge;
                ++i)
            {
                const double t =
                    static_cast<double>(i) /
                    static_cast<double>(
                        segmentsPerEdge);

                points.emplace_back(
                    halfWidth,
                    -halfHeight +
                    height * t);
            }

            // Top: top-right -> top-left
            for (std::uint32_t i = 0;
                i < segmentsPerEdge;
                ++i)
            {
                const double t =
                    static_cast<double>(i) /
                    static_cast<double>(
                        segmentsPerEdge);

                points.emplace_back(
                    halfWidth -
                    width * t,
                    halfHeight);
            }

            // Left: top-left -> bottom-left
            for (std::uint32_t i = 0;
                i < segmentsPerEdge;
                ++i)
            {
                const double t =
                    static_cast<double>(i) /
                    static_cast<double>(
                        segmentsPerEdge);

                points.emplace_back(
                    -halfWidth,
                    halfHeight -
                    height * t);
            }

            return points;
        }


        SampledAperture sampleAperture(
            const optics::Aperture& aperture,
            std::uint32_t perimeterSegments)
        {
            return std::visit(
                [perimeterSegments](
                    const auto& concrete)
                -> SampledAperture
                {
                    using T =
                        std::decay_t<
                        decltype(concrete)>;

                    //
                    // Annular aperture.
                    //
                    if constexpr (
                        HasInnerOuterRadius<T>::value)
                    {
                        if (
                            !std::isfinite(
                                concrete.innerRadius) ||
                            !std::isfinite(
                                concrete.outerRadius) ||
                            concrete.innerRadius < 0.0 ||
                            concrete.outerRadius <= 0.0 ||
                            concrete.innerRadius >=
                            concrete.outerRadius)
                        {
                            throw std::invalid_argument(
                                "Annular aperture radii are invalid.");
                        }

                        SampledAperture result;

                        result.outerBoundary =
                            sampleEllipse(
                                concrete.outerRadius,
                                concrete.outerRadius,
                                perimeterSegments);

                        //
                        // Treat innerRadius == 0 as an ordinary solid circle.
                        //
                        if (concrete.innerRadius > 0.0)
                        {
                            result.innerBoundary =
                                sampleEllipse(
                                    concrete.innerRadius,
                                    concrete.innerRadius,
                                    perimeterSegments);
                        }

                        return result;
                    }

                    //
                    // Elliptical aperture.
                    //
                    // This branch automatically becomes active once an
                    // EllipticalAperture with radiusX/radiusY is added to
                    // ApertureGeometry.
                    //
                    else if constexpr (
                        HasRadiusXY<T>::value)
                    {
                        SampledAperture result;

                        result.outerBoundary =
                            sampleEllipse(
                                concrete.radiusX,
                                concrete.radiusY,
                                perimeterSegments);

                        return result;
                    }

                    //
                    // Rectangular aperture.
                    //
                    else if constexpr (
                        HasWidthHeight<T>::value)
                    {
                        SampledAperture result;

                        result.outerBoundary =
                            sampleRectangle(
                                concrete.width,
                                concrete.height,
                                perimeterSegments);

                        return result;
                    }

                    //
                    // Circular aperture.
                    //
                    else if constexpr (
                        HasRadius<T>::value)
                    {
                        if (
                            !std::isfinite(
                                concrete.radius) ||
                            concrete.radius <= 0.0)
                        {
                            throw std::invalid_argument(
                                "Circular aperture radius must be finite "
                                "and greater than zero.");
                        }

                        SampledAperture result;

                        result.outerBoundary =
                            sampleEllipse(
                                concrete.radius,
                                concrete.radius,
                                perimeterSegments);

                        return result;
                    }

                    else
                    {
                        static_assert(
                            DependentFalse<T>::value,
                            "MirrorMeshGenerator does not know how to "
                            "sample this aperture geometry.");

                        return {};
                    }
                },
                aperture.geometry());
        }


        //
        // =========================================================================
        // Ring construction
        // =========================================================================
        //

        glm::dvec2 ringPoint(
            const SampledAperture& aperture,
            std::uint32_t ring,
            std::uint32_t radialSegments,
            std::size_t perimeterIndex)
        {
            const double fraction =
                static_cast<double>(ring) /
                static_cast<double>(
                    radialSegments);

            if (!aperture.hasHole())
            {
                //
                // Solid centered/star-shaped aperture.
                //
                // Every interior ring is a scaled copy of the outer boundary.
                //
                return
                    aperture.outerBoundary[
                        perimeterIndex] *
                    fraction;
            }

            //
            // Aperture with a central hole.
            //
            // Interpolate corresponding points from the inner contour to the
            // outer contour.
            //
            const glm::dvec2& inner =
                aperture.innerBoundary[
                    perimeterIndex];

            const glm::dvec2& outer =
                aperture.outerBoundary[
                    perimeterIndex];

            return
                inner +
                (outer - inner) *
                fraction;
        }


        //
        // =========================================================================
        // Wall construction
        // =========================================================================
        //

        void appendBoundaryWall(
            MeshData& mesh,
            const std::vector<glm::dvec2>& boundary,
            const optics::SurfaceGeometry& geometry,
            double thickness,
            bool innerWall)
        {
            if (boundary.size() < 3)
            {
                throw std::runtime_error(
                    "Mirror boundary must contain at least three points.");
            }

            const std::size_t count =
                boundary.size();

            //
            // Deliberately generate four separate vertices per edge rather than
            // sharing them.
            //
            // This produces correct hard edges for rectangles/polygons while
            // still looking effectively smooth for sufficiently tessellated
            // circles and ellipses.
            //
            for (std::size_t i = 0;
                i < count;
                ++i)
            {
                const std::size_t next =
                    (i + 1) %
                    count;

                const glm::dvec2& p0 =
                    boundary[i];

                const glm::dvec2& p1 =
                    boundary[next];

                const glm::dvec2 edge =
                    p1 - p0;

                const double edgeLength =
                    std::hypot(
                        edge.x,
                        edge.y);

                if (
                    !std::isfinite(edgeLength) ||
                    edgeLength <= 0.0)
                {
                    throw std::runtime_error(
                        "Mirror aperture contains a degenerate "
                        "boundary edge.");
                }

                //
                // Boundaries are counter-clockwise.
                //
                // For an outer boundary, the outward XY normal is the
                // clockwise 90-degree rotation:
                //
                //     (dx,dy) -> (dy,-dx)
                //
                // For a hole, outward from the substrate points in the
                // opposite direction, into the hole.
                //
                glm::dvec2 outward(
                    edge.y / edgeLength,
                    -edge.x / edgeLength);

                if (innerWall)
                    outward = -outward;

                const glm::vec3 wallNormal(
                    static_cast<float>(
                        outward.x),
                    static_cast<float>(
                        outward.y),
                    0.0f);

                const double z0 =
                    surfaceZAt(
                        geometry,
                        p0,
                        thickness);

                const double z1 =
                    surfaceZAt(
                        geometry,
                        p1,
                        thickness);

                const std::uint32_t base =
                    static_cast<std::uint32_t>(
                        mesh.vertices.size());

                //
                // Front 0
                //
                mesh.vertices.push_back(
                    MeshVertex{
                        glm::vec3(
                            static_cast<float>(p0.x),
                            static_cast<float>(p0.y),
                            static_cast<float>(z0)),
                        wallNormal
                    });

                //
                // Rear 0
                //
                mesh.vertices.push_back(
                    MeshVertex{
                        glm::vec3(
                            static_cast<float>(p0.x),
                            static_cast<float>(p0.y),
                            static_cast<float>(thickness)),
                        wallNormal
                    });

                //
                // Front 1
                //
                mesh.vertices.push_back(
                    MeshVertex{
                        glm::vec3(
                            static_cast<float>(p1.x),
                            static_cast<float>(p1.y),
                            static_cast<float>(z1)),
                        wallNormal
                    });

                //
                // Rear 1
                //
                mesh.vertices.push_back(
                    MeshVertex{
                        glm::vec3(
                            static_cast<float>(p1.x),
                            static_cast<float>(p1.y),
                            static_cast<float>(thickness)),
                        wallNormal
                    });

                const std::uint32_t front0 =
                    base + 0;

                const std::uint32_t rear0 =
                    base + 1;

                const std::uint32_t front1 =
                    base + 2;

                const std::uint32_t rear1 =
                    base + 3;

                if (!innerWall)
                {
                    //
                    // Outer wall winding.
                    //
                    mesh.indices.insert(
                        mesh.indices.end(),
                        {
                            front0, rear1, rear0,
                            front0, front1, rear1
                        });
                }
                else
                {
                    //
                    // Reverse winding for an inner hole wall.
                    //
                    mesh.indices.insert(
                        mesh.indices.end(),
                        {
                            front0, rear0, rear1,
                            front0, rear1, front1
                        });
                }
            }
        }

    } // anonymous namespace


    MeshData MirrorMeshGenerator::generate(
        const optics::OpticalSurface& opticalSurface,
        double thickness,
        std::uint32_t radialSegments,
        std::uint32_t angularSegments)
    {
        //
        // =========================================================================
        // Input validation
        // =========================================================================
        //

        if (
            !std::isfinite(thickness) ||
            thickness <= 0.0)
        {
            throw std::invalid_argument(
                "Mirror thickness must be finite and greater than zero.");
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
        // =========================================================================
        // Sample aperture
        // =========================================================================
        //

        const SampledAperture aperture =
            sampleAperture(
                opticalSurface.aperture(),
                angularSegments);

        if (aperture.outerBoundary.size() < 3)
        {
            throw std::runtime_error(
                "Mirror aperture outer boundary must contain "
                "at least three points.");
        }

        if (
            aperture.hasHole() &&
            aperture.innerBoundary.size() !=
            aperture.outerBoundary.size())
        {
            throw std::runtime_error(
                "Mirror aperture inner and outer contours must have "
                "matching sample counts.");
        }

        const bool hasCentralHole =
            aperture.hasHole();

        const std::uint32_t perimeterCount =
            static_cast<std::uint32_t>(
                aperture.perimeterCount());


        MeshData mesh;


        //
        // =========================================================================
        // Optical face
        // =========================================================================
        //

        std::uint32_t frontCenter = 0;

        if (!hasCentralHole)
        {
            const glm::dvec2 center2D(
                0.0,
                0.0);

            const double centerZ =
                surfaceZAt(
                    opticalSurface.geometry(),
                    center2D,
                    thickness);

            glm::dvec3 centerNormal =
                surfaceNormal(
                    opticalSurface.geometry(),
                    glm::dvec3(
                        0.0,
                        0.0,
                        centerZ));

            //
            // Optical-face outward normal points generally toward -Z.
            //
            if (centerNormal.z > 0.0)
                centerNormal = -centerNormal;

            frontCenter =
                static_cast<std::uint32_t>(
                    mesh.vertices.size());

            mesh.vertices.push_back(
                MeshVertex{
                    glm::vec3(
                        0.0f,
                        0.0f,
                        static_cast<float>(
                            centerZ)),
                    toFloat(
                        centerNormal)
                });
        }


        const std::uint32_t frontRingStart =
            static_cast<std::uint32_t>(
                mesh.vertices.size());

        //
        // Solid apertures start at ring 1 because ring 0 would collapse all
        // perimeter samples to the center.
        //
        // Apertures with holes start at ring 0 because ring 0 is the inner
        // boundary itself.
        //
        const std::uint32_t firstRing =
            hasCentralHole
            ? 0
            : 1;


        for (std::uint32_t ring = firstRing;
            ring <= radialSegments;
            ++ring)
        {
            for (std::uint32_t i = 0;
                i < perimeterCount;
                ++i)
            {
                const glm::dvec2 point =
                    ringPoint(
                        aperture,
                        ring,
                        radialSegments,
                        i);

                const double z =
                    surfaceZAt(
                        opticalSurface.geometry(),
                        point,
                        thickness);

                glm::dvec3 normal =
                    surfaceNormal(
                        opticalSurface.geometry(),
                        glm::dvec3(
                            point.x,
                            point.y,
                            z));

                if (normal.z > 0.0)
                    normal = -normal;

                mesh.vertices.push_back(
                    MeshVertex{
                        glm::vec3(
                            static_cast<float>(
                                point.x),
                            static_cast<float>(
                                point.y),
                            static_cast<float>(
                                z)),
                        toFloat(
                            normal)
                    });
            }
        }


        //
        // Solid mirror center fan.
        //
        if (!hasCentralHole)
        {
            for (std::uint32_t i = 0;
                i < perimeterCount;
                ++i)
            {
                const std::uint32_t next =
                    (i + 1) %
                    perimeterCount;

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
        // Connect adjacent front-face rings.
        //
        for (std::uint32_t ring = 0;
            ring + 1 < frontRingCount;
            ++ring)
        {
            const std::uint32_t innerStart =
                frontRingStart +
                ring *
                perimeterCount;

            const std::uint32_t outerStart =
                frontRingStart +
                (ring + 1) *
                perimeterCount;

            for (std::uint32_t i = 0;
                i < perimeterCount;
                ++i)
            {
                const std::uint32_t next =
                    (i + 1) %
                    perimeterCount;

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
        // Flat rear face
        // =========================================================================
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
            for (std::uint32_t i = 0;
                i < perimeterCount;
                ++i)
            {
                const glm::dvec2 point =
                    ringPoint(
                        aperture,
                        ring,
                        radialSegments,
                        i);

                mesh.vertices.push_back(
                    MeshVertex{
                        glm::vec3(
                            static_cast<float>(
                                point.x),
                            static_cast<float>(
                                point.y),
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
        // Solid mirror rear center fan.
        //
        if (!hasCentralHole)
        {
            for (std::uint32_t i = 0;
                i < perimeterCount;
                ++i)
            {
                const std::uint32_t next =
                    (i + 1) %
                    perimeterCount;

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
        // Connect adjacent rear-face rings.
        //
        for (std::uint32_t ring = 0;
            ring + 1 < rearRingCount;
            ++ring)
        {
            const std::uint32_t innerStart =
                rearRingStart +
                ring *
                perimeterCount;

            const std::uint32_t outerStart =
                rearRingStart +
                (ring + 1) *
                perimeterCount;

            for (std::uint32_t i = 0;
                i < perimeterCount;
                ++i)
            {
                const std::uint32_t next =
                    (i + 1) %
                    perimeterCount;

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
        // Outer substrate wall
        // =========================================================================
        //

        appendBoundaryWall(
            mesh,
            aperture.outerBoundary,
            opticalSurface.geometry(),
            thickness,
            false);


        //
        // =========================================================================
        // Inner substrate wall
        // =========================================================================
        //

        if (hasCentralHole)
        {
            appendBoundaryWall(
                mesh,
                aperture.innerBoundary,
                opticalSurface.geometry(),
                thickness,
                true);
        }


        return mesh;
    }

} // namespace opticforge