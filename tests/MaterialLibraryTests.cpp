// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "optics/MaterialLibrary.h"
#include "project/MaterialLibraryIO.h"
#include "project/MaterialLibrarySerializer.h"
#include "raytracer/ReferenceSpectrum.h"

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

    const double d65Sample =
        raytracer::sampleReferenceSpectrum(
            raytracer::ReferenceSpectrum::D65,
            0.5);

    check(
        d65Sample >= 380.0 &&
        d65Sample <= 780.0,
        "D65 sampler returns visible wavelengths");

    const double a0Median =
        raytracer::sampleReferenceSpectrum(
            raytracer::ReferenceSpectrum::A0,
            0.5);

    const double m0Median =
        raytracer::sampleReferenceSpectrum(
            raytracer::ReferenceSpectrum::M0,
            0.5);

    check(
        a0Median < m0Median,
        "hotter stellar continuum samples bluer wavelengths");

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
