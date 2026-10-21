// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once
#include "optics/Ray.h"
#include "RayInteraction.h"
#include <vector>

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

        // Cumulative optical path length to the final intersection (mm).
        double opticalPathLength = 0.0;

        RayTermination termination =
            RayTermination::Active;
    };
}