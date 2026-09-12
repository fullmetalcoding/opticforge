#pragma once

#include <glm/glm.hpp>

namespace opticforge::optics
{

    struct SurfaceHit
    {
        // Distance along the ray:
        //
        //     P(t) = origin + t * direction
        //
        // Ray directions are expected to be normalized, so t is also
        // the physical distance from the ray origin in project units.
        double t = 0.0;

        // Intersection point in the coordinate space used for the query.
        glm::dvec3 position{ 0.0, 0.0, 0.0 };

        // Unit surface normal at the intersection point.
        glm::dvec3 normal{ 0.0, 0.0, 1.0 };
    };

} // namespace opticforge::optics