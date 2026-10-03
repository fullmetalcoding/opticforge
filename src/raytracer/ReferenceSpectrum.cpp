// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "ReferenceSpectrum.h"
#include "optics/Colorimetry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace opticforge::raytracer
{
    namespace
    {
        struct SpectrumPoint
        {
            double wavelengthNm;
            double relativePower;
        };

        constexpr std::array<SpectrumPoint, 21>
            D65Samples{{
                { 380.0, 49.9755 },
                { 400.0, 82.7549 },
                { 420.0, 93.4318 },
                { 440.0, 104.865 },
                { 460.0, 117.812 },
                { 480.0, 115.923 },
                { 500.0, 109.354 },
                { 520.0, 104.790 },
                { 540.0, 104.405 },
                { 560.0, 100.000 },
                { 580.0, 95.7880 },
                { 600.0, 90.0062 },
                { 620.0, 87.6987 },
                { 640.0, 83.6992 },
                { 660.0, 80.2146 },
                { 680.0, 78.2842 },
                { 700.0, 71.6091 },
                { 720.0, 61.6040 },
                { 740.0, 75.0870 },
                { 760.0, 46.4182 },
                { 780.0, 63.3828 }
            }};

        double planckRelativePower(
            double wavelengthNm,
            double temperatureK)
        {
            constexpr double C2 =
                1.438776877e-2;

            const double wavelengthM =
                wavelengthNm * 1.0e-9;

            const double exponent =
                C2 /
                (wavelengthM * temperatureK);

            if (exponent > 700.0)
                return 0.0;

            const double denominator =
                std::expm1(exponent);

            if (
                !std::isfinite(denominator) ||
                denominator <= 0.0)
            {
                return 0.0;
            }

            // The common 2hc^2 factor cancels during normalization.
            return
                1.0 /
                (
                    std::pow(wavelengthM, 5.0) *
                    denominator
                );
        }

        double d65RelativePower(
            double wavelengthNm)
        {
            if (wavelengthNm <= D65Samples.front().wavelengthNm)
                return D65Samples.front().relativePower;

            if (wavelengthNm >= D65Samples.back().wavelengthNm)
                return D65Samples.back().relativePower;

            const auto upper =
                std::upper_bound(
                    D65Samples.begin(),
                    D65Samples.end(),
                    wavelengthNm,
                    [](double wavelength, const SpectrumPoint& point)
                    {
                        return wavelength < point.wavelengthNm;
                    });

            const auto lower =
                upper - 1;

            const double t =
                (wavelengthNm - lower->wavelengthNm) /
                (upper->wavelengthNm - lower->wavelengthNm);

            return
                lower->relativePower +
                (upper->relativePower - lower->relativePower) *
                t;
        }

        double relativePower(
            ReferenceSpectrum spectrum,
            double wavelengthNm)
        {
            if (spectrum == ReferenceSpectrum::D65)
                return d65RelativePower(wavelengthNm);

            const double temperature =
                referenceSpectrumTemperatureK(
                    spectrum);

            if (temperature <= 0.0)
            {
                throw std::invalid_argument(
                    "Invalid stellar reference spectrum.");
            }

            return
                planckRelativePower(
                    wavelengthNm,
                    temperature);
        }

        struct IntegratedBand
        {
            double power = 0.0;
            double wavelengthMoment = 0.0;
            glm::dvec3 cieXyz{ 0.0 };
        };

        IntegratedBand integrateBand(
            ReferenceSpectrum spectrum,
            double beginNm,
            double endNm)
        {
            if (
                !std::isfinite(beginNm) ||
                !std::isfinite(endNm) ||
                endNm <= beginNm)
            {
                throw std::invalid_argument(
                    "Invalid reference-spectrum band.");
            }

            // At most 1 nm per quadrature interval. This makes total source
            // color effectively independent of whether the visible range is
            // partitioned into 3, 7, 15, or 31 traced bands.
            const std::size_t intervals =
                std::max<std::size_t>(
                    1,
                    static_cast<std::size_t>(
                        std::ceil(
                            endNm -
                            beginNm)));

            const double step =
                (endNm - beginNm) /
                static_cast<double>(
                    intervals);

            IntegratedBand result;

            for (
                std::size_t i = 0;
                i <= intervals;
                ++i)
            {
                const double wavelength =
                    beginNm +
                    step *
                    static_cast<double>(i);

                const double endpointFactor =
                    (i == 0 || i == intervals)
                    ? 0.5
                    : 1.0;

                const double power =
                    relativePower(
                        spectrum,
                        wavelength);

                if (
                    !std::isfinite(power) ||
                    power < 0.0)
                {
                    throw std::runtime_error(
                        "Reference spectrum produced invalid power.");
                }

                const double weightedPower =
                    endpointFactor *
                    power;

                result.power +=
                    weightedPower;

                result.wavelengthMoment +=
                    weightedPower *
                    wavelength;

                result.cieXyz +=
                    optics::cie1931Xyz(
                        wavelength) *
                    weightedPower;
            }

            result.power *=
                step;

            result.wavelengthMoment *=
                step;

            result.cieXyz *=
                step;

            return result;
        }
    }

    const char* referenceSpectrumName(
        ReferenceSpectrum spectrum) noexcept
    {
        switch (spectrum)
        {
        case ReferenceSpectrum::D65: return "CIE D65";
        case ReferenceSpectrum::O5: return "O5 star";
        case ReferenceSpectrum::B0: return "B0 star";
        case ReferenceSpectrum::A0: return "A0 star";
        case ReferenceSpectrum::F0: return "F0 star";
        case ReferenceSpectrum::G0: return "G0 star";
        case ReferenceSpectrum::K0: return "K0 star";
        case ReferenceSpectrum::M0: return "M0 star";
        }

        return "Unknown";
    }

    double referenceSpectrumTemperatureK(
        ReferenceSpectrum spectrum) noexcept
    {
        switch (spectrum)
        {
        case ReferenceSpectrum::O5: return 40000.0;
        case ReferenceSpectrum::B0: return 30000.0;
        case ReferenceSpectrum::A0: return 9600.0;
        case ReferenceSpectrum::F0: return 7300.0;
        case ReferenceSpectrum::G0: return 5940.0;
        case ReferenceSpectrum::K0: return 5250.0;
        case ReferenceSpectrum::M0: return 3850.0;
        case ReferenceSpectrum::D65: return 0.0;
        }

        return 0.0;
    }

    bool isSupportedSpectralSampleCount(
        std::size_t sampleCount) noexcept
    {
        return
            sampleCount == 3 ||
            sampleCount == 7 ||
            sampleCount == 15 ||
            sampleCount == 31;
    }

    std::vector<SpectralSample> buildReferenceSpectrum(
        ReferenceSpectrum spectrum,
        std::size_t sampleCount)
    {
        if (!isSupportedSpectralSampleCount(sampleCount))
        {
            throw std::invalid_argument(
                "Reference spectrum sample count must be 3, 7, 15, or 31.");
        }

        constexpr double MinWavelengthNm =
            380.0;

        constexpr double MaxWavelengthNm =
            780.0;

        const double bandWidth =
            (MaxWavelengthNm - MinWavelengthNm) /
            static_cast<double>(
                sampleCount);

        std::vector<IntegratedBand> bands;
        bands.reserve(sampleCount);

        double totalPower = 0.0;

        for (
            std::size_t i = 0;
            i < sampleCount;
            ++i)
        {
            const double beginNm =
                MinWavelengthNm +
                bandWidth *
                static_cast<double>(i);

            const double endNm =
                i + 1 == sampleCount
                ? MaxWavelengthNm
                : MinWavelengthNm +
                    bandWidth *
                    static_cast<double>(i + 1);

            IntegratedBand band =
                integrateBand(
                    spectrum,
                    beginNm,
                    endNm);

            if (
                !std::isfinite(band.power) ||
                band.power <= 0.0 ||
                !std::isfinite(
                    band.wavelengthMoment))
            {
                throw std::runtime_error(
                    "Reference spectrum produced an empty spectral band.");
            }

            totalPower +=
                band.power;

            bands.push_back(
                band);
        }

        if (
            !std::isfinite(totalPower) ||
            totalPower <= 0.0)
        {
            throw std::runtime_error(
                "Reference spectrum contains no positive visible power.");
        }

        std::vector<SpectralSample> samples;
        samples.reserve(sampleCount);

        for (const auto& band : bands)
        {
            const double representativeWavelength =
                band.wavelengthMoment /
                band.power;

            const glm::dvec3 xyzPerUnitPower =
                band.cieXyz /
                band.power;

            if (
                !std::isfinite(representativeWavelength) ||
                representativeWavelength <= 0.0 ||
                !std::isfinite(xyzPerUnitPower.x) ||
                !std::isfinite(xyzPerUnitPower.y) ||
                !std::isfinite(xyzPerUnitPower.z))
            {
                throw std::runtime_error(
                    "Reference spectrum produced invalid integrated color data.");
            }

            samples.push_back(
                {
                    representativeWavelength,
                    band.power /
                        totalPower,
                    xyzPerUnitPower
                });
        }

        return samples;
    }


} // namespace opticforge::raytracer
