// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "renderer/PsfRasterizer.h"
#include "renderer/PsfRenderController.h"
#include "raytracer/ReferenceSpectrum.h"
#include "optics/Colorimetry.h"

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

    // Equal power must retain the low CIE response at the spectral tails.
    for (const double wavelength : { 300.0, 350.0, 800.0, 850.0 })
    {
        for (const auto background : {
            renderer::PsfBackground::Black, renderer::PsfBackground::White })
        {
            auto invisibleSettings = settings;
            invisibleSettings.background = background;
            const auto invisible = renderer::rasterizePsf(
                makeSingleSpot(wavelength), plane, invisibleSettings);
            const int expected = background == renderer::PsfBackground::Black ? 0 : 255;
            check(centerRgb(invisible) == std::array<int, 3>{expected, expected, expected},
                "UV and IR leave the background unchanged");
        }
    }
    const auto farRed = renderer::rasterizePsf(makeSingleSpot(750.0), plane, settings);
    check(channel(farRed, 0) > 0 && channel(farRed, 1) == 0 && channel(farRed, 2) == 0,
        "750 nm renders dim red rather than green");
    check(channel(farRed, 0) < channel(red, 0) / 10,
        "far-red response is not normalized to full brightness");
    for (const double wavelength : { 700.0, 725.0, 750.0, 775.0, 780.0 })
    {
        const auto linear = optics::xyzToLinearSrgb(optics::cie1931Xyz(wavelength));
        check(linear.x > 0.0 && linear.x > linear.y && linear.x > linear.z,
            "long-wavelength CIE tail stays red-dominant");
    }
    {
        auto cachedInvisible = makeSingleSpot(800.0);
        cachedInvisible.paths.front().interactions.back().incoming.cieXyzPerUnitPower =
            glm::dvec3(0.0);
        check(centerRgb(renderer::rasterizePsf(cachedInvisible, plane, settings)) ==
            std::array<int, 3>{0, 0, 0}, "cached zero XYZ is not replaced with white");
    }
    const auto xyz750 = optics::cie1931Xyz(750.0);
    check(std::abs(xyz750.x - 0.0003323011) < 1e-10 &&
        std::abs(xyz750.y - 0.00012) < 1e-10,
        "750 nm uses tabulated CIE response");
    for (const double wavelength : { 450.0, 550.0, 650.0, 750.0 })
    {
        auto low = settings;
        low.normalizePeak = false;
        low.exposure = 0.01;
        auto high = low;
        high.exposure = 0.02;
        const auto acc = renderer::accumulatePsf(makeSingleSpot(wavelength), plane, low);
        const auto a = centerRgb(renderer::tonemapPsf(acc, low));
        const auto b = centerRgb(renderer::tonemapPsf(acc, high));
        check(b[0] >= a[0] && b[1] >= a[1] && b[2] >= a[2],
            "exposure increases spectral brightness");
    }
    {
        auto mixed = makeSingleSpot(550.0);
        auto invisible = makeSingleSpot(800.0);
        mixed.paths.push_back(invisible.paths.front());
        auto absolute = settings;
        absolute.normalizePeak = false;
        check(renderer::rasterizePsf(mixed, plane, absolute).rgba ==
            renderer::rasterizePsf(makeSingleSpot(550.0), plane, absolute).rgba,
            "invisible power does not contaminate a visible spectrum");
    }

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

        // Progress reporting: monotonic, ends at progressEnd, for the
        // direct, separable and point paths.
        for (const auto& [sigma, mark] : {
            std::pair{ 1.25, renderer::PsfMark::Gaussian },
            std::pair{ 6.0, renderer::PsfMark::Gaussian },
            std::pair{ 1.25, renderer::PsfMark::Point } })
        {
            // 128x128 so sigma = 6 takes the separable path.
            renderer::PsfRenderSettings s;
            s.width = 128;
            s.height = 128;
            s.sigmaPixels = sigma;
            s.mark = mark;

            std::atomic<double> progress{ 0.0 };
            std::atomic<bool> done{ false };
            bool monotonic = true;

            std::thread watcher([&]()
                {
                    double last = 0.0;

                    while (!done.load())
                    {
                        const double now = progress.load();

                        if (now < last || now > 0.5 + 1e-12)
                            monotonic = false;

                        last = now;
                    }
                });

            renderer::PsfExecution execution;
            execution.threads = 4;
            execution.progress = &progress;
            execution.progressEnd = 0.5;

            const auto acc =
                renderer::accumulatePsf(scattered, plane, s, {}, execution);

            done = true;
            watcher.join();

            check(monotonic, "PSF progress never decreases or overshoots");
            check(
                progress.load() == 0.5,
                "accumulatePsf progress ends at progressEnd");

            execution.progressBegin = 0.5;
            execution.progressEnd = 1.0;

            (void)renderer::tonemapPsf(acc, s, execution);

            check(
                progress.load() == 1.0,
                "tonemapPsf progress ends at 1");
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

            // A job that finishes before noticing it was superseded must
            // not be displayed. Force the race: let job A complete, then
            // request B (different sigma) before polling.
            {
                renderer::PsfRenderSettings stale = last;
                stale.sigmaPixels = 4.0;

                renderer::PsfRenderSettings fresh = last;
                fresh.sigmaPixels = 2.5;

                renderer::PsfRenderController racing;
                racing.request(trace, 1, stale);
                racing.waitForRunningJobForTesting();
                racing.request(trace, 1, fresh);

                std::optional<renderer::PsfImage> first;

                while (!first && racing.busy())
                {
                    first = racing.poll();

                    if (!first)
                    {
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(1));
                    }
                }

                check(
                    first &&
                    first->rgba ==
                    renderer::rasterizePsf(scattered, plane, fresh).rgba,
                    "superseded PSF result is never displayed");

                // Same race with a new trace version.
                auto newer = std::make_shared<raytracer::CompletedTrace>();
                newer->result = makeScatteredSpot(3000);
                newer->observationPlane = plane;

                renderer::PsfRenderController racingTrace;
                racingTrace.request(trace, 1, last);
                racingTrace.waitForRunningJobForTesting();
                racingTrace.request(newer, 2, last);

                std::optional<renderer::PsfImage> next;

                while (!next && racingTrace.busy())
                {
                    next = racingTrace.poll();

                    if (!next)
                    {
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(1));
                    }
                }

                check(
                    next &&
                    next->rgba ==
                    renderer::rasterizePsf(
                        newer->result, plane, last).rgba,
                    "result for an older trace is never displayed");

                // Tonemap-only follow-up still shows the in-flight image
                // first, then the re-tonemapped one.
                renderer::PsfRenderSettings dimmer = last;
                dimmer.exposure = 0.5;

                renderer::PsfRenderController racingTone;
                racingTone.request(newer, 2, last);
                racingTone.waitForRunningJobForTesting();
                racingTone.request(newer, 2, dimmer);

                std::optional<renderer::PsfImage> shown;

                while (!(shown = racingTone.poll()))
                {
                }

                check(
                    shown->rgba ==
                    renderer::rasterizePsf(newer->result, plane, last).rgba,
                    "in-flight image is shown for a tonemap-only change");
            }
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
