// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Colorimetry.h"
#include "Cie1931Table.h"

#include <algorithm>
#include <cmath>

namespace opticforge::optics
{
    glm::dvec3 cie1931Xyz(double wavelengthNm) noexcept
    {
        if (!std::isfinite(wavelengthNm) ||
            wavelengthNm < 380.0 || wavelengthNm > 780.0)
            return glm::dvec3(0.0);

        const double offset = wavelengthNm - 380.0;
        const auto lower = static_cast<std::size_t>(offset);
        const auto upper = std::min(lower + 1, Cie1931Table.size() - 1);
        const double fraction = offset - static_cast<double>(lower);
        glm::dvec3 xyz;
        for (std::size_t channel = 0; channel < 3; ++channel)
            xyz[channel] = Cie1931Table[lower][channel] * (1.0 - fraction) +
                Cie1931Table[upper][channel] * fraction;
        return xyz;
    }

    glm::dvec3 xyzToLinearSrgb(
        const glm::dvec3& xyz) noexcept
    {
        return {
            3.2404542 * xyz.x -
            1.5371385 * xyz.y -
            0.4985314 * xyz.z,

            -0.9692660 * xyz.x +
            1.8760108 * xyz.y +
            0.0415560 * xyz.z,

            0.0556434 * xyz.x -
            0.2040259 * xyz.y +
            1.0572252 * xyz.z
        };
    }

    double linearToSrgb(
        double linear) noexcept
    {
        if (!std::isfinite(linear))
            return 0.0;

        const double clamped =
            std::max(
                0.0,
                linear);

        if (clamped <= 0.0031308)
            return 12.92 * clamped;

        return
            1.055 *
            std::pow(
                clamped,
                1.0 / 2.4) -
            0.055;
    }

    glm::dvec3 linearToSrgb(
        const glm::dvec3& linear) noexcept
    {
        return {
            linearToSrgb(linear.x),
            linearToSrgb(linear.y),
            linearToSrgb(linear.z)
        };
    }

} // namespace opticforge::optics
