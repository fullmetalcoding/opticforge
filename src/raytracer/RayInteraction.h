#pragma once
#include "optics/Ray.h"
#include "optics/SurfaceHit.h"

namespace opticforge::raytracer {

    struct RayInteraction
    {
        telescope::PrimitiveId primitiveId;
        optics::SurfaceHit hit;
        optics::OpticalRay incoming;
        optics::OpticalRay outgoing;
    };

  
}
