// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "ReferenceSpectrum.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace opticforge::raytracer
{
    namespace
    {
        struct SpectrumSample
        {
            double wavelengthNm;
            double relativePower;
        };

        constexpr std::array<SpectrumSample, 21>
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

        std::vector<SpectrumSample>
            stellarSamples(
                double temperatureK)
        {
            std::vector<SpectrumSample> result;
            result.reserve(41);

            for (
                int wavelength = 380;
                wavelength <= 780;
                wavelength += 10)
            {
                result.push_back(
                    {
                        static_cast<double>(wavelength),
                        planckRelativePower(
                            static_cast<double>(wavelength),
                            temperatureK)
                    });
            }

            return result;
        }

        template <typename Range>
        double sampleWeighted(
            const Range& samples,
            double u)
        {
            if (
                !std::isfinite(u) ||
                u < 0.0 ||
                u >= 1.0)
            {
                throw std::invalid_argument(
                    "Spectrum sample coordinate must be in [0, 1).");
            }

            double total = 0.0;

            for (const auto& sample : samples)
            {
                if (
                    std::isfinite(sample.relativePower) &&
                    sample.relativePower > 0.0)
                {
                    total +=
                        sample.relativePower;
                }
            }

            if (
                !std::isfinite(total) ||
                total <= 0.0)
            {
                throw std::runtime_error(
                    "Reference spectrum contains no positive power.");
            }

            const double target =
                u * total;

            double cumulative = 0.0;

            for (const auto& sample : samples)
            {
                cumulative +=
                    std::max(
                        0.0,
                        sample.relativePower);

                if (target <= cumulative)
                    return sample.wavelengthNm;
            }

            return
                samples.back().wavelengthNm;
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

    double sampleReferenceSpectrum(
        ReferenceSpectrum spectrum,
        double u)
    {
        if (spectrum == ReferenceSpectrum::D65)
        {
            return
                sampleWeighted(
                    D65Samples,
                    u);
        }

        const double temperature =
            referenceSpectrumTemperatureK(
                spectrum);

        if (temperature <= 0.0)
        {
            throw std::invalid_argument(
                "Invalid stellar reference spectrum.");
        }

        return
            sampleWeighted(
                stellarSamples(temperature),
                u);
    }

} // namespace opticforge::raytracer
