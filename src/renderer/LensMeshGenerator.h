// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

// LensMeshGenerator.h

#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "optics/OpticalSurface.h"
#include "MeshData.h"

namespace opticforge
{


    class LensMeshGenerator
    {
    public:
        static MeshData generate(
            const optics::OpticalSurface& frontSurface,
            const optics::OpticalSurface& rearSurface,
            double thickness,
            std::uint32_t radialSegments = 32,
            std::uint32_t angularSegments = 96);
    };

} // namespace opticforge