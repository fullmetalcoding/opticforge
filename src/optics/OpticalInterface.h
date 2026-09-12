#pragma once

#include <cstdint>
#include <variant>

namespace opticforge::optics
{

    //
    // Temporary material identifier.
    //
    // Once Material.h exists, I would move MaterialId there and include
    // Material.h from this file instead.
    //
    using MaterialId = std::uint64_t;


    //
    // Refractive boundary between two optical media.
    //
    // The sides are defined relative to the OpticalSurface's local normal.
    //
    // For the canonical surface orientation, the normal points toward the
    // positive side.
    //
    // The ray tracer determines which material the ray is leaving and
    // entering from the sign of dot(ray.direction, surfaceNormal).
    //
    struct RefractiveInterface
    {
        double negativeSideMaterial = 0;
        double positiveSideMaterial = 0;

        RefractiveInterface() = default;

        RefractiveInterface(
            double negativeSideMaterial,
            double positiveSideMaterial);
    };


    //
    // Reflective optical boundary.
    //
    // reflectivity is represented as a fraction:
    //
    //     0.0 = completely absorbing
    //     1.0 = perfectly reflective
    //
    // This is intentionally wavelength-independent for now. A coating
    // model can replace or augment this later.
    //
    struct ReflectiveInterface
    {
        double reflectivity = 1.0;

        ReflectiveInterface() = default;

        explicit ReflectiveInterface(
            double reflectivity);
    };


    //
    // Detector surface.
    //
    // A ray reaching this interface is recorded as a detector hit and
    // normally terminates.
    //
    struct DetectorInterface
    {
    };


    //
    // Fully absorbing surface.
    //
    // A ray reaching this interface terminates without producing another
    // propagating ray.
    //
    struct AbsorbingInterface
    {
    };


    //
    // Closed set of optical boundary behaviors currently supported by
    // Opticforge.
    //
    using OpticalInterfaceType =
        std::variant<
        RefractiveInterface,
        ReflectiveInterface,
        DetectorInterface,
        AbsorbingInterface>;


    //
    // Describes the physical interaction that occurs when a ray reaches
    // an OpticalSurface.
    //
    // Geometry determines WHERE the ray intersects.
    // OpticalInterface determines WHAT happens at that intersection.
    //
    class OpticalInterface
    {
    public:
        //
        // Default to a completely absorbing surface. This is a safer
        // default than accidentally treating an unspecified surface as
        // reflective or refractive.
        //
        OpticalInterface();

        explicit OpticalInterface(
            const RefractiveInterface& interface);

        explicit OpticalInterface(
            const ReflectiveInterface& interface);

        explicit OpticalInterface(
            const DetectorInterface& interface);

        explicit OpticalInterface(
            const AbsorbingInterface& interface);

        explicit OpticalInterface(
            const OpticalInterfaceType& interface);

        const OpticalInterfaceType& type() const;
        OpticalInterfaceType& type();

        void setType(
            const OpticalInterfaceType& interface);

        void setRefractive(
            double negativeSideMaterial,
            double positiveSideMaterial);

        void setReflective(
            double reflectivity = 1.0);

        void setDetector();

        void setAbsorbing();

        bool isRefractive() const;
        bool isReflective() const;
        bool isDetector() const;
        bool isAbsorbing() const;

    private:
        OpticalInterfaceType m_type;
    };

} // namespace opticforge::optics