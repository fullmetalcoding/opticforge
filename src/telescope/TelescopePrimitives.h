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