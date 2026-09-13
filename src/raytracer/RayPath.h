#pragma once
#include "optics/Ray.h"
#include "RayInteraction.h"

namespace opticforge::raytracer {
    enum class RayTermination
    {
        Active,
        DetectorHit,
        Absorbed,
        Missed,
        Blocked,
        Escaped,
        TotalInternalReflection,
        MaxInteractions,
        InvalidState
    };
    struct RayPath
    {
        optics::OpticalRay initialRay;

        std::vector<RayInteraction> interactions;

        RayTermination termination =
            RayTermination::Active;
    };
}