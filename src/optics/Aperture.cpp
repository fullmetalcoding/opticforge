#include "Aperture.h"

#include <algorithm>
#include <cmath>

namespace opticforge::optics
{

    // -----------------------------------------------------------------------------
    // CircularAperture
    // -----------------------------------------------------------------------------

    CircularAperture::CircularAperture(
        double radius_)
        : radius(std::max(0.0, radius_))
    {
    }

    bool CircularAperture::contains(
        const glm::dvec2& point) const
    {
        const double r2 =
            point.x * point.x +
            point.y * point.y;

        return r2 <= radius * radius;
    }


    // -----------------------------------------------------------------------------
    // AnnularAperture
    // -----------------------------------------------------------------------------

    AnnularAperture::AnnularAperture(
        double innerRadius_,
        double outerRadius_)
        : innerRadius(
            std::max(0.0, innerRadius_)),
        outerRadius(
            std::max(0.0, outerRadius_))
    {
        if (innerRadius > outerRadius)
            std::swap(innerRadius, outerRadius);
    }

    bool AnnularAperture::contains(
        const glm::dvec2& point) const
    {
        const double r2 =
            point.x * point.x +
            point.y * point.y;

        const double inner2 =
            innerRadius * innerRadius;

        const double outer2 =
            outerRadius * outerRadius;

        return
            r2 >= inner2 &&
            r2 <= outer2;
    }


    // -----------------------------------------------------------------------------
    // RectangularAperture
    // -----------------------------------------------------------------------------

    RectangularAperture::RectangularAperture(
        double width_,
        double height_)
        : width(std::max(0.0, width_)),
        height(std::max(0.0, height_))
    {
    }

    bool RectangularAperture::contains(
        const glm::dvec2& point) const
    {
        const double halfWidth =
            width * 0.5;

        const double halfHeight =
            height * 0.5;

        return
            std::abs(point.x) <= halfWidth &&
            std::abs(point.y) <= halfHeight;
    }


    // -----------------------------------------------------------------------------
    // Aperture
    // -----------------------------------------------------------------------------

    Aperture::Aperture()
        : m_geometry(
            CircularAperture{ 1.0 })
    {
    }

    Aperture::Aperture(
        const CircularAperture& aperture)
        : m_geometry(aperture)
    {
    }

    Aperture::Aperture(
        const AnnularAperture& aperture)
        : m_geometry(aperture)
    {
    }

    Aperture::Aperture(
        const RectangularAperture& aperture)
        : m_geometry(aperture)
    {
    }

    Aperture::Aperture(
        const ApertureGeometry& geometry)
        : m_geometry(geometry)
    {
    }

    bool Aperture::contains(
        const glm::dvec2& point) const
    {
        return std::visit(
            [&](const auto& aperture)
            {
                return aperture.contains(point);
            },
            m_geometry);
    }

    const ApertureGeometry&
        Aperture::geometry() const
    {
        return m_geometry;
    }

    ApertureGeometry&
        Aperture::geometry()
    {
        return m_geometry;
    }

    void Aperture::setGeometry(
        const ApertureGeometry& geometry)
    {
        m_geometry = geometry;
    }

    void Aperture::setCircular(
        double radius)
    {
        m_geometry =
            CircularAperture{
                radius
        };
    }

    void Aperture::setAnnular(
        double innerRadius,
        double outerRadius)
    {
        m_geometry =
            AnnularAperture{
                innerRadius,
                outerRadius
        };
    }

    void Aperture::setRectangular(
        double width,
        double height)
    {
        m_geometry =
            RectangularAperture{
                width,
                height
        };
    }

} // namespace opticforge::optics