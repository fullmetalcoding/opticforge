// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <string>
#include <variant>

namespace opticforge::optics
{

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
        std::string negativeSideMaterial = "opticforge:vacuum";
        std::string positiveSideMaterial = "opticforge:vacuum";

        RefractiveInterface() = default;

        RefractiveInterface(
            std::string negativeSideMaterial,
            std::string positiveSideMaterial);
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
    // Idealized reflective diffraction grating.
    //
    // grooveAngleDegrees is measured in the surface-local XY plane from
    // local +Y toward local +X. Order 0 reduces to specular reflection.
    //
    struct DiffractionGratingInterface
    {
        double groovesPerMm = 600.0;
        int order = 1;
        double grooveAngleDegrees = 0.0;
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
        DiffractionGratingInterface,
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
            const DiffractionGratingInterface& interface);

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
            std::string negativeSideMaterial,
            std::string positiveSideMaterial);

        void setReflective(
            double reflectivity = 1.0);

        void setDiffractionGrating(
            double groovesPerMm,
            int order = 1,
            double grooveAngleDegrees = 0.0);

        void setDetector();

        void setAbsorbing();

        bool isRefractive() const;
        bool isReflective() const;
        bool isDiffractionGrating() const;
        bool isDetector() const;
        bool isAbsorbing() const;

    private:
        OpticalInterfaceType m_type;
    };

} // namespace opticforge::optics