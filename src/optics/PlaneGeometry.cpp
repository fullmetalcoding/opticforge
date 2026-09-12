#include "PlaneGeometry.h"

#include <cmath>
#include <limits>

namespace opticforge::optics
{

    bool PlaneGeometry::intersect(
        const Ray& localRay,
        SurfaceHit& localHit) const
    {
        constexpr double epsilon = 1.0e-12;

        // Plane equation:
        //
        //     z = 0
        //
        // Ray:
        //
        //     P(t) = O + tD
        //
        // Therefore:
        //
        //     Oz + t Dz = 0
        //
        //     t = -Oz / Dz

        if (std::abs(localRay.direction.z) < epsilon)
        {
            // Ray is parallel (or effectively parallel) to the plane.
            return false;
        }

        const double t =
            -localRay.origin.z /
            localRay.direction.z;

        if (t <= epsilon)
        {
            // Intersection lies behind the ray origin, or is too close
            // to safely distinguish from the current surface.
            return false;
        }

        localHit.t = t;
        localHit.position =
            localRay.pointAt(t);

        localHit.normal =
            glm::dvec3(0.0, 0.0, 1.0);

        return true;
    }

    glm::dvec3 PlaneGeometry::normalAt(
        const glm::dvec3& /* localPoint */) const
    {
        return glm::dvec3(
            0.0,
            0.0,
            1.0);
    }

} // namespace opticforge::optics