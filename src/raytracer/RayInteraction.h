// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once
#include "optics/Ray.h"
#include "optics/SurfaceHit.h"
#include "telescope/TelescopePrimitives.h"
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
