// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <glm/glm.hpp>

namespace opticforge::optics
{
    // Approximate CIE 1931 2-degree standard-observer color-matching
    // functions at a monochromatic wavelength in nanometres.
    //
    // The analytic fit follows Wyman, Sloan, and Shirley (JCGT 2013),
    // "Simple Analytic Approximations to the CIE XYZ Color Matching
    // Functions". Values outside 380-780 nm are treated as non-visible.
    glm::dvec3 cie1931Xyz(
        double wavelengthNm) noexcept;

    // Convert CIE XYZ (D65 reference white) to linear sRGB.
    // Components can legitimately be negative for monochromatic colors
    // outside the sRGB gamut; callers decide how to gamut-map/clamp them.
    glm::dvec3 xyzToLinearSrgb(
        const glm::dvec3& xyz) noexcept;

    // IEC 61966-2-1 sRGB transfer function.
    double linearToSrgb(
        double linear) noexcept;

    glm::dvec3 linearToSrgb(
        const glm::dvec3& linear) noexcept;

} // namespace opticforge::optics
