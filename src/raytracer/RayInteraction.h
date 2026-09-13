#pragma once
#include "optics/Ray.h"
#include "optics/SurfaceHit.h"
#include <optional>

namespace opticforge::raytracer {

    struct RayInteraction
    {
        telescope::PrimitiveId primitiveId;
        optics::SurfaceHit hit;
        optics::OpticalRay incoming;
        std::optional<optics::OpticalRay> outgoing;
    };

  
}
