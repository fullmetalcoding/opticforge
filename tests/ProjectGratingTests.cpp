// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "project/EmbeddedProjectSchema.h"
#include "project/ProjectSerializer.h"

#include <cmath>
#include <exception>
#include <iostream>
#include <string_view>

#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>

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
    using namespace opticforge;
    using nlohmann::json;
    using nlohmann::json_schema::json_validator;

    telescope::TelescopeProject project;

    telescope::DiffractionGrating grating;

    grating.transform.setPosition(
        { 1.0, 2.0, 3.0 });

    grating.transform.setEulerDegrees(
        { 4.0, 5.0, 6.0 });

    grating.surface.setGeometry(
        optics::PlaneGeometry{});

    grating.surface.aperture().setRectangular(
        80.0,
        40.0);

    grating.surface.opticalInterface().setDiffractionGrating(
        1200.0,
        -1,
        12.5);

    const auto id =
        project.addPrimitive(
            grating,
            "Test Grating");

    const json document =
        project::ProjectSerializer::serialize(
            project);

    check(
        document.at("version") ==
            project::ProjectSerializer::FormatVersion,
        "grating project uses current format version");

    const std::string_view schemaText =
        project::projectSchemaJson();

    json_validator validator;

    try
    {
        const json schema =
            json::parse(
                schemaText.begin(),
                schemaText.end());

        validator.set_root_schema(
            schema);

        validator.validate(
            document);
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "FAIL: serialized grating project does not validate: "
            << e.what()
            << '\n';

        ++failures;
    }

    telescope::TelescopeProject restored;

    try
    {
        restored =
            project::ProjectSerializer::deserialize(
                document);
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "FAIL: grating project deserialize threw: "
            << e.what()
            << '\n';

        ++failures;
    }

    const auto* primitive =
        restored.findPrimitive(id);

    check(
        primitive != nullptr,
        "round-trip preserves grating primitive id");

    if (primitive)
    {
        const auto* restoredGrating =
            std::get_if<
                telescope::DiffractionGrating>(
                    primitive);

        check(
            restoredGrating != nullptr,
            "round-trip preserves diffraction grating primitive type");

        if (restoredGrating)
        {
            const auto* interface =
                std::get_if<
                    optics::DiffractionGratingInterface>(
                        &restoredGrating->
                        surface.opticalInterface().type());

            check(
                interface != nullptr,
                "round-trip preserves diffraction grating interface");

            if (interface)
            {
                check(
                    std::abs(
                        interface->groovesPerMm -
                        1200.0) <
                        1.0e-12,
                    "round-trip preserves groove density");

                check(
                    interface->order == -1,
                    "round-trip preserves diffraction order");

                check(
                    std::abs(
                        interface->grooveAngleDegrees -
                        12.5) <
                        1.0e-12,
                    "round-trip preserves groove angle");
            }

            const auto* aperture =
                std::get_if<
                    optics::RectangularAperture>(
                        &restoredGrating->
                        surface.aperture().geometry());

            check(
                aperture != nullptr,
                "round-trip preserves rectangular grating aperture");

            if (aperture)
            {
                check(
                    std::abs(aperture->width - 80.0) <
                        1.0e-12 &&
                    std::abs(aperture->height - 40.0) <
                        1.0e-12,
                    "round-trip preserves grating aperture dimensions");
            }
        }
    }

    if (failures)
    {
        std::cerr
            << failures
            << " assertion(s) failed\n";
    }

    return failures ? 1 : 0;
}
