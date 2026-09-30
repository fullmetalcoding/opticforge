// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "renderer/PsfRasterizer.h"
#include "raytracer/ReferenceSpectrum.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <utility>

namespace
{
    int failures = 0;

    void check(
        bool condition,
        const char* message)
    {
        if (!condition)
        {
            std::cerr
                << "FAIL: "
                << message
                << '\n';

            ++failures;
        }
    }

    opticforge::raytracer::TraceResult
        makeSingleSpot(
            double wavelengthNm)
    {
        using namespace opticforge;

        raytracer::TraceResult result;
        raytracer::RayPath path;

        path.termination =
            raytracer::RayTermination::DetectorHit;

        raytracer::RayInteraction prior;
        prior.primitiveId = 42;

        raytracer::RayInteraction hit;
        hit.primitiveId = 0;
        hit.hit.position =
            glm::dvec3(0.0, 0.0, 0.0);

        hit.incoming.wavelength =
            wavelengthNm;

        hit.incoming.intensity =
            1.0;

        path.interactions.push_back(
            prior);

        path.interactions.push_back(
            hit);

        result.paths.push_back(
            std::move(path));

        return result;
    }

    std::uint8_t channel(
        const opticforge::renderer::PsfImage& image,
        int channelIndex)
    {
        const std::size_t x =
            static_cast<std::size_t>(
                image.width / 2);

        const std::size_t y =
            static_cast<std::size_t>(
                image.height / 2);

        const std::size_t index =
            (
                y *
                static_cast<std::size_t>(
                    image.width) +
                x
            ) * 4 +
            static_cast<std::size_t>(
                channelIndex);

        return image.rgba.at(index);
    }

    opticforge::raytracer::TraceResult
        makeBroadbandSpot(
            opticforge::raytracer::ReferenceSpectrum spectrum,
            std::size_t sampleCount)
    {
        using namespace opticforge;

        raytracer::TraceResult result;

        const auto spectralSamples =
            raytracer::buildReferenceSpectrum(
                spectrum,
                sampleCount);

        result.paths.reserve(
            spectralSamples.size());

        for (const auto& sample : spectralSamples)
        {
            raytracer::RayPath path;

            path.termination =
                raytracer::RayTermination::DetectorHit;

            raytracer::RayInteraction prior;
            prior.primitiveId = 42;

            raytracer::RayInteraction hit;
            hit.primitiveId = 0;
            hit.hit.position =
                glm::dvec3(0.0);

            hit.incoming.wavelength =
                sample.wavelengthNm;

            hit.incoming.intensity =
                sample.weight;

            hit.incoming.cieXyzPerUnitPower =
                sample.cieXyzPerUnitPower;

            path.interactions.push_back(
                prior);

            path.interactions.push_back(
                hit);

            result.paths.push_back(
                std::move(path));
        }

        return result;
    }

    std::array<int, 3> centerRgb(
        const opticforge::renderer::PsfImage& image)
    {
        return {
            static_cast<int>(
                channel(image, 0)),
            static_cast<int>(
                channel(image, 1)),
            static_cast<int>(
                channel(image, 2))
        };
    }

    int maxChannelDifference(
        const std::array<int, 3>& a,
        const std::array<int, 3>& b)
    {
        return std::max(
            {
                std::abs(a[0] - b[0]),
                std::abs(a[1] - b[1]),
                std::abs(a[2] - b[2])
            });
    }
}

int main()
{
    using namespace opticforge;

    telescope::ObservationPlane plane;

    renderer::PsfRenderSettings settings;
    settings.width = 16;
    settings.height = 16;
    settings.background =
        renderer::PsfBackground::Black;
    settings.mark =
        renderer::PsfMark::Point;
    settings.autoFit = false;
    settings.autoCenter = false;
    settings.center =
        glm::dvec2(0.0);
    settings.fieldWidth = 2.0;
    settings.normalizePeak = true;
    settings.exposure = 1.0;

    const auto blue =
        renderer::rasterizePsf(
            makeSingleSpot(450.0),
            plane,
            settings);

    const auto red =
        renderer::rasterizePsf(
            makeSingleSpot(650.0),
            plane,
            settings);

    check(
        blue.observationHits == 1 &&
        red.observationHits == 1,
        "PSF accepts wavelength test spots");

    check(
        channel(blue, 2) >
        channel(blue, 0),
        "450 nm spot is blue-dominant");

    check(
        channel(red, 0) >
        channel(red, 2),
        "650 nm spot is red-dominant");

    check(
        channel(blue, 0) !=
            channel(red, 0) ||
        channel(blue, 1) !=
            channel(red, 1) ||
        channel(blue, 2) !=
            channel(red, 2),
        "different wavelengths produce different PSF colors");

    const std::array<std::size_t, 4>
        sampleCounts{ 3, 7, 15, 31 };

    for (
        const auto spectrum :
        {
            raytracer::ReferenceSpectrum::D65,
            raytracer::ReferenceSpectrum::O5,
            raytracer::ReferenceSpectrum::M0
        })
    {
        std::array<int, 3> referenceRgb{};
        bool haveReference = false;

        for (const std::size_t sampleCount : sampleCounts)
        {
            const auto image =
                renderer::rasterizePsf(
                    makeBroadbandSpot(
                        spectrum,
                        sampleCount),
                    plane,
                    settings);

            const auto rgb =
                centerRgb(image);

            if (!haveReference)
            {
                referenceRgb =
                    rgb;

                haveReference =
                    true;
            }
            else
            {
                check(
                    maxChannelDifference(
                        referenceRgb,
                        rgb) <= 2,
                    "broadband PSF hue is stable across spectral sample counts");
            }
        }
    }

    const auto d65Rgb =
        centerRgb(
            renderer::rasterizePsf(
                makeBroadbandSpot(
                    raytracer::ReferenceSpectrum::D65,
                    31),
                plane,
                settings));

    check(
        std::max(
            {
                d65Rgb[0],
                d65Rgb[1],
                d65Rgb[2]
            }) -
        std::min(
            {
                d65Rgb[0],
                d65Rgb[1],
                d65Rgb[2]
            }) <= 5,
        "co-located D65 spectrum renders near neutral white");

    if (failures != 0)
    {
        std::cerr
            << failures
            << " assertion(s) failed\n";
    }

    return
        failures == 0
        ? 0
        : 1;
}
