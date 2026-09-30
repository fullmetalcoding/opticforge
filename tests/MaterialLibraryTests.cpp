// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "optics/MaterialLibrary.h"
#include "project/MaterialLibraryIO.h"
#include "project/MaterialLibrarySerializer.h"

#include <cmath>
#include <filesystem>
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
}

int main()
{
    namespace fs = std::filesystem;
    using namespace opticforge;

    optics::MaterialLibrary library;

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
