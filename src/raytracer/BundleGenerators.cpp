// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "BundleGenerators.h"
#include "optics/Colorimetry.h"

#include <cmath>
#include <cstddef>
#include <limits>
#include <random>
#include <stdexcept>

#include <glm/glm.hpp>

namespace opticforge::raytracer
{
    namespace
    {
        constexpr double TwoPi =
            6.28318530717958647692;
    }

    RayBundle generatePupilRayBundle(
        std::size_t numberOfRays,
        double pupilDiameter,
        double pupilDistanceMinusZ,
        double pupilElevationY,
        double offAxisAngleXRadians,
        double offAxisAngleYRadians,
        const RayBundleSpectrum& spectrum,
        std::uint64_t randomSeed)
    {
        if (numberOfRays == 0)
            return {};

        if (
            !std::isfinite(pupilDiameter) ||
            pupilDiameter <= 0.0)
        {
            throw std::invalid_argument(
                "pupilDiameter must be greater than zero.");
        }

        if (
            !std::isfinite(pupilDistanceMinusZ) ||
            pupilDistanceMinusZ < 0.0)
        {
            throw std::invalid_argument(
                "pupilDistanceMinusZ must be non-negative.");
        }

        if (
            spectrum.mode == SpectrumMode::Monochromatic &&
            (!std::isfinite(spectrum.wavelengthNm) ||
             spectrum.wavelengthNm <= 0.0))
        {
            throw std::invalid_argument(
                "Monochromatic wavelength must be finite and positive.");
        }

        std::vector<SpectralSample> spectralSamples;

        if (spectrum.mode == SpectrumMode::Reference)
        {
            spectralSamples =
                buildReferenceSpectrum(
                    spectrum.reference,
                    spectrum.spectralSampleCount);
        }
        else
        {
            spectralSamples.push_back(
                {
                    spectrum.wavelengthNm,
                    1.0,
                    optics::cie1931Xyz(
                        spectrum.wavelengthNm)
                });
        }

        if (
            numberOfRays >
            std::numeric_limits<std::size_t>::max() /
            spectralSamples.size())
        {
            throw std::overflow_error(
                "Expanded ray bundle size overflows size_t.");
        }

        const std::size_t expandedRayCount =
            numberOfRays *
            spectralSamples.size();

        RayBundle bundle;
        bundle.reserve(expandedRayCount);

        const double pupilRadius =
            pupilDiameter * 0.5;

        const glm::dvec3 pupilCenter(
            0.0,
            pupilElevationY,
            -pupilDistanceMinusZ);

        const glm::dvec3 rayDirection =
            glm::normalize(
                glm::dvec3(
                    std::tan(offAxisAngleXRadians),
                    std::tan(offAxisAngleYRadians),
                    1.0));

        std::mt19937_64 rng(
            randomSeed);

        std::uniform_real_distribution<double>
            unitDistribution(0.0, 1.0);

        for (
            std::size_t i = 0;
            i < numberOfRays;
            ++i)
        {
            const double u =
                unitDistribution(rng);

            const double v =
                unitDistribution(rng);

            const double radius =
                pupilRadius *
                std::sqrt(u);

            const double theta =
                TwoPi *
                v;

            const double x =
                radius *
                std::cos(theta);

            const double y =
                radius *
                std::sin(theta);

            const glm::dvec3 origin =
                pupilCenter +
                glm::dvec3(
                    x,
                    y,
                    0.0);

            for (
                const auto& spectralSample :
                spectralSamples)
            {
                optics::OpticalRay opticalRay;

                opticalRay.ray =
                    optics::Ray(
                        origin,
                        rayDirection);

                opticalRay.wavelength =
                    spectralSample.wavelengthNm;

                opticalRay.intensity =
                    spectralSample.weight;

                opticalRay.cieXyzPerUnitPower =
                    spectralSample.cieXyzPerUnitPower;

                bundle.push_back(
                    std::move(opticalRay));
            }
        }

        return bundle;
    }
}
