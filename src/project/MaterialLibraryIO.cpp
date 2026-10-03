// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "MaterialLibraryIO.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>

#include "EmbeddedMaterialSchema.h"
#include "MaterialLibrarySerializer.h"

namespace opticforge::project
{
    namespace
    {
        using nlohmann::json;
        using nlohmann::json_schema::json_validator;

        const json& materialSchema()
        {
            static const json schema = []
            {
                const std::string_view text =
                    materialSchemaJson();

                return
                    json::parse(
                        text.begin(),
                        text.end());
            }();

            return schema;
        }

        void validate(
            const json& document)
        {
            json_validator validator;
            validator.set_root_schema(
                materialSchema());

            validator.validate(
                document);
        }

        std::string pathForMessage(
            const std::filesystem::path& path)
        {
            return path.string();
        }
    }

    optics::MaterialLibrary MaterialLibraryIO::load(
        const std::filesystem::path& path)
    {
        std::ifstream input(
            path,
            std::ios::binary);

        if (!input)
        {
            throw std::runtime_error(
                "Unable to open material library '" +
                pathForMessage(path) +
                "' for reading.");
        }

        json document;

        try
        {
            input >> document;
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(
                "Unable to parse material library '" +
                pathForMessage(path) +
                "': " +
                e.what());
        }

        try
        {
            validate(document);

            return
                MaterialLibrarySerializer::deserialize(
                    document);
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(
                "Invalid material library '" +
                pathForMessage(path) +
                "': " +
                e.what());
        }
    }

    void MaterialLibraryIO::save(
        const optics::MaterialLibrary& library,
        const std::filesystem::path& path)
    {
        json document =
            MaterialLibrarySerializer::serialize(
                library);

        try
        {
            validate(document);
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(
                std::string(
                    "Material library does not satisfy the OpticForge schema: ") +
                e.what());
        }

        std::ofstream output(
            path,
            std::ios::binary |
            std::ios::trunc);

        if (!output)
        {
            throw std::runtime_error(
                "Unable to open material library '" +
                pathForMessage(path) +
                "' for writing.");
        }

        output
            << document.dump(4)
            << '\n';

        if (!output)
        {
            throw std::runtime_error(
                "Failed while writing material library '" +
                pathForMessage(path) +
                "'.");
        }
    }

} // namespace opticforge::project
