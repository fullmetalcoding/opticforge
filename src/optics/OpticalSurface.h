#pragma once

#include <variant>

#include <glm/glm.hpp>

#include "Transform.h"
#include "Ray.h"

#include "PlaneGeometry.h"
#include "ConicGeometry.h"
//#include "AsphericGeometry.h"

#include "Aperture.h"
#include "OpticalInterface.h"

namespace opticforge::optics
{


    using SurfaceGeometry =
        std::variant<
        PlaneGeometry,
        ConicGeometry>;
      //  AsphericGeometry>;

    class OpticalSurface
    {
    public:
        OpticalSurface() = default;

        OpticalSurface(
            const Transform& transform,
            const SurfaceGeometry& geometry,
            const Aperture& aperture,
            const OpticalInterface& opticalInterface);

        // Transform
        const Transform& transform() const;
        Transform& transform();

        void setTransform(
            const Transform& transform);

        // Geometry
        const SurfaceGeometry& geometry() const;
        SurfaceGeometry& geometry();

        void setGeometry(
            const SurfaceGeometry& geometry);

        // Aperture
        const Aperture& aperture() const;
        Aperture& aperture();

        void setAperture(
            const Aperture& aperture);

        // Optical interface
        const OpticalInterface& opticalInterface() const;
        OpticalInterface& opticalInterface();

        void setOpticalInterface(
            const OpticalInterface& opticalInterface);

        // Intersect a world-space ray with this surface.
        //
        // This function is responsible for:
        //   1. Transforming the ray into surface-local coordinates.
        //   2. Intersecting the underlying SurfaceGeometry.
        //   3. Rejecting hits outside the aperture.
        //   4. Transforming the resulting position and normal back into
        //      world coordinates.
        //
        // Returns true if the ray intersects the usable optical portion
        // of the surface.
        bool intersect(
            const Ray& worldRay,
            SurfaceHit& worldHit,
            double tMin = 1e-9) const;

    private:
        Transform m_transform;

        SurfaceGeometry m_geometry;

        Aperture m_aperture;

        OpticalInterface m_opticalInterface;
    };

} // namespace opticforge::optics