// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once
#include "optics/Ray.h"
#include "RayInteraction.h"
#include <vector>
#include <optional>
#include <cmath>

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

        // OPD against an ideal spherical reference centered at the selected
        // image point. Populated only when an explicit reference is supplied.
        std::optional<double> opticalPathDifferenceMm;

        RayTermination termination =
            RayTermination::Active;
    };
}