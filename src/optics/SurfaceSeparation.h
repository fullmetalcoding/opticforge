// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <optional>

#include "OpticalSurface.h"

namespace opticforge::optics
{
    // Returns the minimum positive axial vertex separation needed
    // to keep rearGeometry strictly behind frontGeometry over
    // [innerRadius, outerRadius].
    //
    // nullopt means one of the surfaces is not defined over the
    // requested radial interval.
    std::optional<double> minimumAxialSeparation(
        const SurfaceGeometry& frontGeometry,
        const SurfaceGeometry& rearGeometry,
        double innerRadius,
        double outerRadius);
}