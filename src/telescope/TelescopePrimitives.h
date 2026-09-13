#pragma once
#include "optics/Transform.h"
#include "optics/OpticalSurface.h"

namespace opticforge::telescope
{
    struct Lens
    {
        optics::Transform transform;
        optics::OpticalSurface frontSurface;
        optics::OpticalSurface rearSurface;
        double centerThickness = 0.0;
    };

    struct Mirror
    {
        optics::Transform transform;
        optics::OpticalSurface surface;
        double thickness = 0.0;
        double centralHole = 0.0; 
    };

    struct Detector
    {
        optics::Transform transform;
        optics::OpticalSurface surface;

        int widthPixels = 0;
        int heightPixels = 0;       
        double pixelPitchUm = 0.0;
    };
    struct LaunchPupil
    {
        optics::Transform transform;
        optics::Aperture aperture;

        // Direction in pupil-local coordinates.
        // All generated rays are parallel for a source at infinity.
        glm::dvec3 localDirection{ 0.0, 0.0, 1.0 };
    };

    struct ObservationPlane
    {
        optics::Transform transform;
        optics::OpticalSurface surface;

        // Full displayed width and height, in project units.
        glm::vec2 displaySize{ 50.0, 50.0 };

        // Independent of the displayed rectangle.
        bool infiniteExtent = true;

        // Accept rays traveling toward the surface's local +Z side.
        bool positiveCrossingOnly = true;

        ObservationPlane()
        {
            surface.setGeometry(optics::PlaneGeometry{});
            surface.opticalInterface().setDetector();
        }
    };
        
    using TelescopePrimitive =
        std::variant<
        Lens,
        Mirror,
        Detector>;

    using PrimitiveId = std::uint64_t;

    struct PrimitiveRecord
    {
        PrimitiveId id;
        TelescopePrimitive primitive;
    };
}