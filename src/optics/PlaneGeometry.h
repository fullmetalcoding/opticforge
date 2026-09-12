#pragma once

#include "Ray.h"
#include "SurfaceHit.h"

namespace opticforge::optics
{

    class PlaneGeometry
    {
    public:
        PlaneGeometry() = default;

        // The canonical plane is:
        //
        //     z = 0
        //
        // with its nominal normal pointing along +Z.
        //
        // Placement and orientation in the telescope are handled by
        // OpticalSurface::Transform, not by PlaneGeometry itself.

        bool intersect(
            const Ray& localRay,
            SurfaceHit& localHit) const;

        glm::dvec3 normalAt(
            const glm::dvec3& localPoint) const;
    };

} // namespace opticforge::optics