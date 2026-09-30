// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "ReferenceSpectrum.h"

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

        const double step =
            (MaxWavelengthNm - MinWavelengthNm) /
            static_cast<double>(sampleCount - 1);

        std::vector<SpectralSample> samples;
        samples.reserve(sampleCount);

        double totalWeight = 0.0;

        for (
            std::size_t i = 0;
            i < sampleCount;
            ++i)
        {
            const double wavelength =
                MinWavelengthNm +
                step *
                static_cast<double>(i);

            double weight =
                relativePower(
                    spectrum,
                    wavelength);

            // Trapezoidal integration over an evenly spaced wavelength grid.
            // The common wavelength step cancels when weights are normalized.
            if (i == 0 || i + 1 == sampleCount)
                weight *= 0.5;

            if (
                !std::isfinite(weight) ||
                weight < 0.0)
            {
                throw std::runtime_error(
                    "Reference spectrum produced an invalid spectral weight.");
            }

            samples.push_back(
                {
                    wavelength,
                    weight
                });

            totalWeight +=
                weight;
        }

        if (
            !std::isfinite(totalWeight) ||
            totalWeight <= 0.0)
        {
            throw std::runtime_error(
                "Reference spectrum contains no positive visible power.");
        }

        for (auto& sample : samples)
            sample.weight /= totalWeight;

        return samples;
    }

} // namespace opticforge::raytracer
