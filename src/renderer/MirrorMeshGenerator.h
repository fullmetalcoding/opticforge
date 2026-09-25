// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstdint>

#include "MeshData.h"
#include "optics/OpticalSurface.h"

namespace opticforge
{
   
    class MirrorMeshGenerator
    {
    public:
        //
        // Generates a closed solid mirror mesh.
        //
        // Convention:
        //
        //   optical surface vertex: z = 0
        //   substrate extends toward +Z
        //   flat rear surface:       z = thickness
        //
        // All dimensions are in the optics model's native units
        // (currently millimeters).
        //
        static MeshData generate(
            const optics::OpticalSurface& opticalSurface,
            double thickness,
            std::uint32_t radialSegments = 32,
            std::uint32_t angularSegments = 96);
    };

} // namespace opticforge