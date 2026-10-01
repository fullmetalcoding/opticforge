// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "renderer/PsfRasterizer.h"
#include "renderer/PsfRenderController.h"
#include "raytracer/ReferenceSpectrum.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <utility>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <optional>
#include <thread>

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

    // Many rays spread over a few hundred pixels, with lateral colour, so
    // footprints cross row-band boundaries.
    opticforge::raytracer::TraceResult
        makeScatteredSpot(
            std::size_t count)
    {
        using namespace opticforge;

        raytracer::TraceResult result;
        result.paths.reserve(count);

        std::uint64_t state = 0x9E3779B97F4A7C15ull;

        const auto next = [&state]()
            {
                state ^= state << 13;
                state ^= state >> 7;
                state ^= state << 17;
                return
                    static_cast<double>(state >> 11) /
                    9007199254740992.0;
            };

        for (std::size_t i = 0; i < count; ++i)
        {
            raytracer::RayPath path;

            path.termination =
                raytracer::RayTermination::DetectorHit;

            raytracer::RayInteraction prior;
            prior.primitiveId = 42;

            raytracer::RayInteraction hit;
            hit.primitiveId = 0;

            const double wavelength = 420.0 + 260.0 * next();

            hit.hit.position = glm::dvec3(
                (next() - 0.5) * 0.05 + (wavelength - 550.0) * 1e-4,
                (next() - 0.5) * 0.05,
                0.0);

            hit.incoming.wavelength = wavelength;
            hit.incoming.intensity = 0.25 + next();

            path.interactions.push_back(prior);
            path.interactions.push_back(hit);
            result.paths.push_back(std::move(path));
        }

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

    // ------------------------------------------------------------------
    // Parallel / async rasterization.
    // ------------------------------------------------------------------
    {
        const auto scattered = makeScatteredSpot(5000);

        for (const double sigma : { 1.25, 3.0 })
        {
            renderer::PsfRenderSettings s;
            s.width = 160;
            s.height = 128;
            s.sigmaPixels = sigma;

            renderer::PsfExecution exact1;
            exact1.threads = 1;
            exact1.allowSeparable = false;

            renderer::PsfExecution exact4 = exact1;
            exact4.threads = 4;

            const auto serial =
                renderer::rasterizePsf(scattered, plane, s, {}, exact1);

            const auto parallel =
                renderer::rasterizePsf(scattered, plane, s, {}, exact4);

            check(
                serial.rgba == parallel.rgba &&
                serial.center == parallel.center &&
                serial.fieldSize == parallel.fieldSize,
                "PSF output is identical for 1 and 4 threads");

            renderer::PsfExecution fast;
            fast.threads = 4;

            const auto approx =
                renderer::rasterizePsf(scattered, plane, s, {}, fast);

            int maxDiff = 0;

            for (std::size_t i = 0; i < serial.rgba.size(); ++i)
            {
                maxDiff = std::max(
                    maxDiff,
                    std::abs(
                        static_cast<int>(serial.rgba[i]) -
                        static_cast<int>(approx.rgba[i])));
            }

            check(
                maxDiff <= 2,
                "separable large-sigma path matches exact splatting");
        }

        // Tonemap-only settings reuse an accumulation.
        {
            renderer::PsfRenderSettings a;
            renderer::PsfRenderSettings b = a;
            b.exposure = 3.0;
            b.background = renderer::PsfBackground::White;
            b.normalizePeak = false;

            check(
                renderer::psfAccumulationSettingsEqual(a, b),
                "exposure/background/normalize do not invalidate accumulation");

            b.sigmaPixels = 2.0;

            check(
                !renderer::psfAccumulationSettingsEqual(a, b),
                "sigma invalidates accumulation");
        }

        // Cancellation.
        {
            std::atomic<bool> cancel{ true };

            renderer::PsfExecution execution;
            execution.cancel = &cancel;

            bool cancelled = false;

            try
            {
                (void)renderer::rasterizePsf(
                    scattered, plane, {}, {}, execution);
            }
            catch (const renderer::PsfCancelled&)
            {
                cancelled = true;
            }

            check(cancelled, "a set cancel flag aborts rasterization");
        }

        // Background controller: latest request wins and the result
        // matches a synchronous render of that request.
        {
            auto trace = std::make_shared<raytracer::CompletedTrace>();
            trace->result = scattered;
            trace->observationPlane = plane;

            renderer::PsfRenderSettings first;
            first.sigmaPixels = 6.0;

            renderer::PsfRenderSettings last;
            last.sigmaPixels = 1.5;
            last.exposure = 2.0;

            renderer::PsfRenderController controller;
            controller.request(trace, 1, first);
            controller.request(trace, 1, last);

            std::optional<renderer::PsfImage> latest;

            const auto deadline =
                std::chrono::steady_clock::now() +
                std::chrono::seconds(20);

            while (
                controller.busy() &&
                std::chrono::steady_clock::now() < deadline)
            {
                if (auto image = controller.poll())
                    latest = std::move(image);

                std::this_thread::sleep_for(
                    std::chrono::milliseconds(1));
            }

            if (auto image = controller.poll())
                latest = std::move(image);

            const auto expected =
                renderer::rasterizePsf(scattered, plane, last);

            check(
                latest && latest->rgba == expected.rgba,
                "async controller delivers the latest request's image");

            // Tonemap-only follow-up.
            renderer::PsfRenderSettings brighter = last;
            brighter.exposure = 5.0;

            controller.request(trace, 1, brighter);

            std::optional<renderer::PsfImage> retoned;

            while (!retoned)
                retoned = controller.poll();

            check(
                retoned->rgba ==
                renderer::rasterizePsf(scattered, plane, brighter).rgba,
                "tonemap-only change reuses accumulation correctly");
        }
    }

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
