// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Colorimetry.h"

#include <algorithm>
#include <cmath>

namespace opticforge::optics
{
    namespace
    {
        double gaussian(
            double value)
        {
            return
                std::exp(
                    -0.5 *
                    value *
                    value);
        }
    }

    glm::dvec3 cie1931Xyz(
        double wavelengthNm) noexcept
    {
        if (
            !std::isfinite(wavelengthNm) ||
            wavelengthNm < 380.0 ||
            wavelengthNm > 780.0)
        {
            return glm::dvec3(0.0);
        }

        // Wyman/Sloan/Shirley asymmetric-Gaussian fit to the
        // CIE 1931 2-degree standard-observer curves.
        const double xT1 =
            (wavelengthNm - 442.0) *
            (wavelengthNm < 442.0
                ? 0.0624
                : 0.0374);

        const double xT2 =
            (wavelengthNm - 599.8) *
            (wavelengthNm < 599.8
                ? 0.0264
                : 0.0323);

        const double xT3 =
            (wavelengthNm - 501.1) *
            (wavelengthNm < 501.1
                ? 0.0490
                : 0.0382);

        const double x =
            0.362 * gaussian(xT1) +
            1.056 * gaussian(xT2) -
            0.065 * gaussian(xT3);

        const double yT1 =
            (wavelengthNm - 568.8) *
            (wavelengthNm < 568.8
                ? 0.0213
                : 0.0247);

        const double yT2 =
            (wavelengthNm - 530.9) *
            (wavelengthNm < 530.9
                ? 0.0613
                : 0.0322);

        const double y =
            0.821 * gaussian(yT1) +
            0.286 * gaussian(yT2);

        const double zT1 =
            (wavelengthNm - 437.0) *
            (wavelengthNm < 437.0
                ? 0.0845
                : 0.0278);

        const double zT2 =
            (wavelengthNm - 459.0) *
            (wavelengthNm < 459.0
                ? 0.0385
                : 0.0725);

        const double z =
            1.217 * gaussian(zT1) +
            0.681 * gaussian(zT2);

        return glm::max(
            glm::dvec3(
                x,
                y,
                z),
            glm::dvec3(0.0));
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
