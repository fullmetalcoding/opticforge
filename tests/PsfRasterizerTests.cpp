// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "renderer/PsfRasterizer.h"

#include <cstdint>
#include <iostream>

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
