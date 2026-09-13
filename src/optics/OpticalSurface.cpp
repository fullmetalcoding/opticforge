#include "OpticalSurface.h"

#include <utility>

namespace opticforge::optics
{

    OpticalSurface::OpticalSurface(
        const Transform& transform,
        const SurfaceGeometry& geometry,
        const Aperture& aperture,
        const OpticalInterface& opticalInterface)
        : m_transform(transform),
        m_geometry(geometry),
        m_aperture(aperture),
        m_opticalInterface(opticalInterface)
    {
    }

    const Transform& OpticalSurface::transform() const
    {
        return m_transform;
    }

    Transform& OpticalSurface::transform()
    {
        return m_transform;
    }

    void OpticalSurface::setTransform(
        const Transform& transform)
    {
        m_transform = transform;
    }

    const SurfaceGeometry& OpticalSurface::geometry() const
    {
        return m_geometry;
    }

    SurfaceGeometry& OpticalSurface::geometry()
    {
        return m_geometry;
    }

    void OpticalSurface::setGeometry(
        const SurfaceGeometry& geometry)
    {
        m_geometry = geometry;
    }

    const Aperture& OpticalSurface::aperture() const
    {
        return m_aperture;
    }

    Aperture& OpticalSurface::aperture()
    {
        return m_aperture;
    }

    void OpticalSurface::setAperture(
        const Aperture& aperture)
    {
        m_aperture = aperture;
    }

    const OpticalInterface&
        OpticalSurface::opticalInterface() const
    {
        return m_opticalInterface;
    }

    OpticalInterface&
        OpticalSurface::opticalInterface()
    {
        return m_opticalInterface;
    }

    void OpticalSurface::setOpticalInterface(
        const OpticalInterface& opticalInterface)
    {
        m_opticalInterface = opticalInterface;
    }

    bool OpticalSurface::intersect(
        const Ray& worldRay,
        SurfaceHit& worldHit,
        double tMin) const
    {
        //
        // Geometry classes operate entirely in the surface's canonical
        // local coordinate system.
        //
        // Convert the incoming world-space ray to local coordinates.
        //
        const Ray localRay(
            m_transform.worldToLocalPoint(
                worldRay.origin),

            m_transform.worldToLocalDirection(
                worldRay.direction));

        SurfaceHit localHit;

        //
        // Dispatch to whichever concrete geometry type is currently
        // stored in the SurfaceGeometry variant.
        //
        const bool hit =
            std::visit(
                [&](const auto& geometry)
                {
                    return geometry.intersect(
                        localRay,
                        localHit,
                        tMin);
                },
                m_geometry);

        if (!hit)
            return false;

        //
        // The canonical optical surface is rotationally oriented around
        // local +Z, so aperture coordinates lie in the local XY plane.
        //
        // Reject intersections that hit the mathematical surface but
        // fall outside its usable optical aperture.
        //
        if (!m_aperture.contains(
            glm::dvec2(
                localHit.position.x,
                localHit.position.y)))
        {
            return false;
        }

        //
        // Convert the accepted intersection back to world coordinates.
        //
        worldHit.position =
            m_transform.localToWorldPoint(
                localHit.position);

        worldHit.normal =
            m_transform.localToWorldNormal(
                localHit.normal);

        //
        // Transform is rigid-body only: rotation + translation, no scale.
        //
        // Therefore ray distances are preserved between local and world
        // coordinates, so the local t remains valid in world space.
        //
        worldHit.t = localHit.t;

        return true;
    }

} // namespace opticforge::optics