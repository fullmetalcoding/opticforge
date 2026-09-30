// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "optics/MaterialLibrary.h"
#include "project/MaterialLibraryIO.h"
#include "project/MaterialLibrarySerializer.h"
#include "raytracer/ReferenceSpectrum.h"
#include "raytracer/BundleGenerators.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>

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
}

int main(int argc, char** argv)
{
    namespace fs = std::filesystem;
    using namespace opticforge;

    optics::MaterialLibrary library;

    check(
        std::abs(
            library.refractiveIndex(
                "opticforge:vacuum",
                550.0) -
            1.0) <
            1.0e-12,
        "vacuum is available as a built-in optical medium");

    library.metadata().id =
        "unit-test";

    library.metadata().name =
        "Unit Test Library";

    library.metadata().revision =
        "42";

    optics::Material material;
    material.key =
        "unit-test:N-BK7";

    material.name =
        "N-BK7";

    material.category =
        optics::MaterialCategory::OpticalGlass;

    optics::SellmeierDispersion sellmeier;
    sellmeier.B =
        {
            1.03961212,
            0.231792344,
            1.01046945
        };

    sellmeier.C =
        {
            0.00600069867,
            0.0200179144,
            103.560653
        };

    sellmeier.validWavelengthNm =
        optics::WavelengthRangeNm{
            300.0,
            2500.0
        };

    material.optics =
        optics::IsotropicOptics{
            sellmeier,
            std::nullopt
        };

    check(
        library.add(material),
        "material can be added");

    check(
        !library.add(material),
        "duplicate material key is rejected");

    const double blueIndex =
        library.refractiveIndex(
            "unit-test:N-BK7",
            486.13);

    const double redIndex =
        library.refractiveIndex(
            "unit-test:N-BK7",
            656.27);

    check(
        blueIndex > redIndex,
        "Sellmeier material disperses blue more strongly than red");

    check(
        std::abs(
            library.refractiveIndex(
                "legacy-index:1.5168",
                550.0) -
            1.5168) <
            1.0e-12,
        "legacy constant-index material references remain readable");

    const auto document =
        project::MaterialLibrarySerializer::serialize(
            library);

    check(
        document.at("format") ==
            "OpticForgeMaterialLibrary",
        "serializer writes material library format");

    check(
        document.at("version") == 1,
        "serializer writes material library version");

    const auto restored =
        project::MaterialLibrarySerializer::deserialize(
            document);

    check(
        restored.metadata().id ==
            "unit-test",
        "library metadata round-trips");

    check(
        restored.materials().size() == 1,
        "material count round-trips");

    const auto* restoredMaterial =
        restored.find(
            "unit-test:N-BK7");

    check(
        restoredMaterial != nullptr,
        "material key round-trips");

    if (restoredMaterial)
    {
        check(
            restoredMaterial->name ==
                "N-BK7",
            "material name round-trips");

        const auto* isotropic =
            std::get_if<
                optics::IsotropicOptics>(
                    &restoredMaterial->optics);

        check(
            isotropic != nullptr,
            "material optical symmetry round-trips");

        if (isotropic)
        {
            const auto* restoredSellmeier =
                std::get_if<
                    optics::SellmeierDispersion>(
                        &isotropic->refractiveIndex);

            check(
                restoredSellmeier != nullptr,
                "Sellmeier model round-trips");

            if (restoredSellmeier)
            {
                check(
                    restoredSellmeier->B.size() == 3 &&
                    restoredSellmeier->C.size() == 3,
                    "Sellmeier coefficients round-trip");

                check(
                    std::abs(
                        restoredSellmeier->B[0] -
                        1.03961212) <
                        1.0e-12,
                    "Sellmeier coefficient value round-trips");
            }
        }
    }

    raytracer::RayBundleSpectrum monoSpectrum;
    monoSpectrum.mode =
        raytracer::SpectrumMode::Monochromatic;
    monoSpectrum.wavelengthNm =
        532.0;

    const auto monoBundle =
        raytracer::generatePupilRayBundle(
            64,
            100.0,
            10.0,
            0.0,
            0.0,
            0.0,
            monoSpectrum,
            1234);

    check(
        monoBundle.size() == 64,
        "monochromatic bundle preserves requested ray count");

    bool monoWavelengthsCorrect =
        true;

    for (const auto& ray : monoBundle)
    {
        monoWavelengthsCorrect =
            monoWavelengthsCorrect &&
            std::abs(ray.wavelength - 532.0) <
                1.0e-12;
    }

    check(
        monoWavelengthsCorrect,
        "monochromatic bundle uses one requested wavelength");

    raytracer::RayBundleSpectrum referenceSpectrum;
    referenceSpectrum.mode =
        raytracer::SpectrumMode::Reference;
    referenceSpectrum.reference =
        raytracer::ReferenceSpectrum::D65;
    referenceSpectrum.spectralSampleCount =
        7;

    const std::size_t pupilSamples =
        32;

    const auto referenceBundle =
        raytracer::generatePupilRayBundle(
            pupilSamples,
            100.0,
            10.0,
            0.0,
            0.0,
            0.0,
            referenceSpectrum,
            1234);

    check(
        referenceBundle.size() ==
            pupilSamples *
            referenceSpectrum.spectralSampleCount,
        "reference-spectrum bundle expands each pupil sample across wavelengths");

    bool sharedOrigin =
        referenceBundle.size() >=
        referenceSpectrum.spectralSampleCount;

    double firstGroupWeight =
        0.0;

    bool firstGroupHasDifferentWavelengths =
        false;

    if (sharedOrigin)
    {
        const auto& origin =
            referenceBundle.front().ray.origin;

        const double firstWavelength =
            referenceBundle.front().wavelength;

        for (
            std::size_t i = 0;
            i < referenceSpectrum.spectralSampleCount;
            ++i)
        {
            const auto& ray =
                referenceBundle[i];

            sharedOrigin =
                sharedOrigin &&
                ray.ray.origin.x == origin.x &&
                ray.ray.origin.y == origin.y &&
                ray.ray.origin.z == origin.z;

            firstGroupWeight +=
                ray.intensity;

            if (
                std::abs(
                    ray.wavelength -
                    firstWavelength) >
                1.0e-12)
            {
                firstGroupHasDifferentWavelengths =
                    true;
            }
        }
    }

    check(
        sharedOrigin,
        "all wavelengths reuse the same Monte Carlo pupil point");

    check(
        firstGroupHasDifferentWavelengths,
        "one pupil point is traced at multiple wavelengths");

    check(
        std::abs(firstGroupWeight - 1.0) <
            1.0e-12,
        "spectral ray intensities are normalized per pupil point");

    const auto referenceBundleRepeat =
        raytracer::generatePupilRayBundle(
            pupilSamples,
            100.0,
            10.0,
            0.0,
            0.0,
            0.0,
            referenceSpectrum,
            1234);

    bool deterministic =
        referenceBundle.size() ==
        referenceBundleRepeat.size();

    for (
        std::size_t i = 0;
        deterministic &&
        i < referenceBundle.size();
        ++i)
    {
        const auto& a =
            referenceBundle[i];

        const auto& b =
            referenceBundleRepeat[i];

        deterministic =
            a.wavelength == b.wavelength &&
            a.intensity == b.intensity &&
            a.ray.origin.x == b.ray.origin.x &&
            a.ray.origin.y == b.ray.origin.y &&
            a.ray.origin.z == b.ray.origin.z;
    }

    check(
        deterministic,
        "reference-spectrum bundle is deterministic for a fixed seed");

    const auto d65Samples =
        raytracer::buildReferenceSpectrum(
            raytracer::ReferenceSpectrum::D65,
            7);

    double d65WeightSum =
        0.0;

    for (const auto& sample : d65Samples)
        d65WeightSum += sample.weight;

    check(
        d65Samples.size() == 7 &&
        std::abs(d65WeightSum - 1.0) <
            1.0e-12,
        "D65 discrete spectral weights are normalized");

    const auto a0Samples =
        raytracer::buildReferenceSpectrum(
            raytracer::ReferenceSpectrum::A0,
            15);

    const auto m0Samples =
        raytracer::buildReferenceSpectrum(
            raytracer::ReferenceSpectrum::M0,
            15);

    const auto weightedMeanWavelength =
        [](const auto& samples)
        {
            double result = 0.0;

            for (const auto& sample : samples)
            {
                result +=
                    sample.wavelengthNm *
                    sample.weight;
            }

            return result;
        };

    check(
        weightedMeanWavelength(a0Samples) <
        weightedMeanWavelength(m0Samples),
        "hotter stellar continuum has a bluer weighted mean wavelength");

    const fs::path path =
        fs::temp_directory_path() /
        "opticforge_material_library_test.ofmat";

    try
    {
        project::MaterialLibraryIO::save(
            library,
            path);

        const auto loaded =
            project::MaterialLibraryIO::load(
                path);

        check(
            loaded.find(
                "unit-test:N-BK7") != nullptr,
            ".ofmat save/load round-trips through schema validation");
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "FAIL: .ofmat IO threw: "
            << e.what()
            << '\n';

        ++failures;
    }

    if (argc > 1)
    {
        try
        {
            const auto starter =
                project::MaterialLibraryIO::load(
                    fs::path(argv[1]));

            check(
                starter.find(
                    "opticforge:vacuum") != nullptr,
                "starter library contains vacuum");

            check(
                starter.find(
                    "opticforge:air") != nullptr,
                "starter library contains standard optical air");

            check(
                starter.find(
                    "schott:N-BK7") != nullptr,
                "starter library contains N-BK7");

            check(
                starter.find(
                    "schott:N-F2") != nullptr,
                "starter library contains N-F2");

            check(
                starter.find(
                    "ohara:S-FPL53") != nullptr,
                "starter library contains S-FPL53");

            check(
                starter.find(
                    "schott:N-LAK22") != nullptr,
                "starter library contains N-LAK22");

            const double nd =
                starter.refractiveIndex(
                    "schott:N-BK7",
                    587.56);

            check(
                std::abs(nd - 1.5168) < 5.0e-4,
                "starter N-BK7 dispersion evaluates near catalog nd");
        }
        catch (const std::exception& e)
        {
            std::cerr
                << "FAIL: starter library load threw: "
                << e.what()
                << '\n';

            ++failures;
        }
    }

    std::error_code removeError;
    fs::remove(
        path,
        removeError);

    if (failures)
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
