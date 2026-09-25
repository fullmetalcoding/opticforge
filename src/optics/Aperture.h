// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <variant>

#include <glm/glm.hpp>

namespace opticforge::optics
{
    struct EllipticalAperture
    {
        double radiusX = 1.0;
        double radiusY = 1.0;

        EllipticalAperture() = default;

        EllipticalAperture(
            double radiusX,
            double radiusY);

        bool contains(
            const glm::dvec2& point) const;
    };

    //
    // Circular aperture centered on the local optical axis.
    //
    struct CircularAperture
    {
        double radius = 1.0;

        CircularAperture() = default;

        explicit CircularAperture(double radius);

        bool contains(
            const glm::dvec2& point) const;
    };


    //
    // Annular aperture centered on the local optical axis.
    //
    // Useful for geometry that has both an inner and outer physical
    // boundary.
    //
    struct AnnularAperture
    {
        double innerRadius = 0.0;
        double outerRadius = 1.0;

        AnnularAperture() = default;

        AnnularAperture(
            double innerRadius,
            double outerRadius);

        bool contains(
            const glm::dvec2& point) const;
    };


    //
    // Rectangular aperture centered on the local optical axis.
    //
    // Width is along local X.
    // Height is along local Y.
    //
    struct RectangularAperture
    {
        double width = 1.0;
        double height = 1.0;

        RectangularAperture() = default;

        RectangularAperture(
            double width,
            double height);

        bool contains(
            const glm::dvec2& point) const;
    };


    //
    // Closed set of aperture geometries supported by Opticforge.
    //
    using ApertureGeometry =
        std::variant<
        CircularAperture,
        AnnularAperture,
        RectangularAperture,
        EllipticalAperture>;


    //
    // Finite usable region of an OpticalSurface.
    //
    // Surface geometry itself is mathematically unbounded. Aperture
    // determines whether a local-space surface intersection lies within
    // the physical/usable part of that surface.
    //
    // Aperture coordinates are always expressed in the OpticalSurface's
    // local XY plane.
    //
    class Aperture
    {
    public:
        //
        // Default to a unit circular aperture.
        //
        Aperture();

        explicit Aperture(
            const CircularAperture& aperture);

        explicit Aperture(
            const AnnularAperture& aperture);

        explicit Aperture(
            const RectangularAperture& aperture);

        explicit Aperture(
            const ApertureGeometry& geometry);

        explicit Aperture(
            const EllipticalAperture& aperture);


        //
        // Returns true if a local-space XY point lies within
        // the aperture.
        //
        bool contains(
            const glm::dvec2& point) const;

        //
        // Underlying aperture geometry.
        //
        const ApertureGeometry& geometry() const;
        ApertureGeometry& geometry();

        void setGeometry(
            const ApertureGeometry& geometry);

        void setCircular(
            double radius);

        void setAnnular(
            double innerRadius,
            double outerRadius);

        void setRectangular(
            double width,
            double height);
       
        void setElliptical(
            double radiusX,
            double radiusY);

    private:
        ApertureGeometry m_geometry;
    };

} // namespace opticforge::optics