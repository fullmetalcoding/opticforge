// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "project/EmbeddedMaterialSchema.h"

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
    using nlohmann::json;
    using nlohmann::json_schema::json_validator;


    //
    // This exercises the complete embedding path:
    //
    // resources/material-schema.json
    //      -> CMake byte generation
    //      -> generated/EmbeddedMaterialSchema.cpp
    //      -> materialSchemaJson()
    //
    const std::string_view schemaText =
        opticforge::project::materialSchemaJson();


    check(
        !schemaText.empty(),
        "embedded material schema is not empty");


    json schema;

    try
    {
        schema =
            json::parse(
                schemaText.begin(),
                schemaText.end());
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "FAIL: embedded material schema is not valid JSON: "
            << e.what()
            << '\n';

        ++failures;
    }


    if (!schema.is_null())
    {
        check(
            schema.value(
                "title",
                std::string{}) ==
                "OpticForge Material Library",
            "embedded schema has the expected title");


        check(
            schema.contains("definitions"),
            "embedded schema contains definitions");
    }


    json_validator validator;

    try
    {
        validator.set_root_schema(
            schema);
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "FAIL: material schema validator rejected embedded schema: "
            << e.what()
            << '\n';

        ++failures;
    }


    //
    // Exercise references inside the schema rather than validating only an
    // empty material list. This document reaches libraryMetadata, material,
    // opticalProperties, isotropicOptics, refractiveIndexModel, and
    // constantDispersion.
    //
    const json validLibrary = {
        {
            "format",
            "OpticForgeMaterialLibrary"
        },
        {
            "version",
            1
        },
        {
            "library",
            {
                { "id", "unit-test" },
                { "name", "Unit Test Material Library" },
                { "revision", "1" }
            }
        },
        {
            "materials",
            json::array({
                {
                    {
                        "key",
                        "unit-test:test-glass"
                    },
                    {
                        "name",
                        "Test Glass"
                    },
                    {
                        "optics",
                        {
                            { "type", "isotropic" },
                            {
                                "refractiveIndex",
                                {
                                    { "type", "constant" },
                                    { "refractiveIndex", 1.5 }
                                }
                            }
                        }
                    }
                }
            })
        }
    };


    try
    {
        validator.validate(
            validLibrary);
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "FAIL: embedded material schema rejected a valid library: "
            << e.what()
            << '\n';

        ++failures;
    }


    //
    // Verify that validation is actually active. Removing the required optics
    // object from a material must be rejected.
    //
    json invalidLibrary =
        validLibrary;

    invalidLibrary["materials"][0].erase(
        "optics");


    bool invalidRejected = false;

    try
    {
        validator.validate(
            invalidLibrary);
    }
    catch (const std::exception&)
    {
        invalidRejected = true;
    }


    check(
        invalidRejected,
        "embedded material schema rejects an invalid material library");


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
