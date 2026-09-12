#pragma once

#include <glm/glm.hpp>

#include "Ray.h"
#include "SurfaceHit.h"

namespace opticforge::optics
{

    class ConicGeometry
    {
    public:
        ConicGeometry() = default;

        ConicGeometry(
            double radiusOfCurvature,
            double conicConstant);

        double radiusOfCurvature() const;
        void setRadiusOfCurvature(
            double radiusOfCurvature);

        double conicConstant() const;
        void setConicConstant(
            double conicConstant);

        // Intersects a ray with the canonical rotationally symmetric
        // conic whose vertex is at the origin and whose axis is +Z.
        //
        // This geometry is expressed entirely in local coordinates.
        // OpticalSurface is responsible for transforming world-space
        // rays into this local coordinate system.
        bool intersect(
            const Ray& localRay,
            SurfaceHit& localHit) const;

        // Returns the local-space unit normal at a point known to lie
        // on the conic.
        glm::dvec3 normalAt(
            const glm::dvec3& localPoint) const;

        // Returns the sag z(r) of the surface at radial distance r.
        //
        // r is measured from the local +Z optical axis:
        //
        //     r^2 = x^2 + y^2
        //
        double sag(double r) const;

    private:
        double m_radiusOfCurvature = 1.0;
        double m_conicConstant = 0.0;
    };

} // namespace opticforge::optics