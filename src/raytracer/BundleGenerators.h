// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "optics/Ray.h"
#include "ReferenceSpectrum.h"

namespace opticforge::raytracer
{
    using RayBundle =
        std::vector<optics::OpticalRay>;

    struct RayBundleSpectrum
    {
        SpectrumMode mode =
            SpectrumMode::Monochromatic;

        double wavelengthNm = 550.0;

        ReferenceSpectrum reference =
            ReferenceSpectrum::D65;
    };

    RayBundle generatePupilRayBundle(
        std::size_t numberOfRays,
        double pupilDiameter,
        double pupilDistanceMinusZ,
        double pupilElevationY,
        double offAxisAngleXRadians,
        double offAxisAngleYRadians,
        const RayBundleSpectrum& spectrum = {},
        std::uint64_t randomSeed = 1);
}
