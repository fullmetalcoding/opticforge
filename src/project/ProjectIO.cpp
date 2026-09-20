#include "project/ProjectIO.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>

#include "project/EmbeddedProjectSchema.h"
#include "project/ProjectSerializer.h"
#include <iostream>


namespace opticforge::project
{
    namespace
    {

        using nlohmann::json;
        using nlohmann::json_schema::json_validator;

        const json& embeddedSchema()
        {
            static const json schema = []
                {
                    try
                    {
                        const std::string_view schemaText =
                            projectSchemaJson();

                        return json::parse(
                            schemaText.begin(),
                            schemaText.end());
                    }
                    catch (const std::exception& e)
                    {
                        throw std::logic_error(
                            std::string(
                                "The embedded OpticForge project schema "
                                "is invalid JSON: ") +
                            e.what());
                    }
                }();

            return schema;
        }

        void validateDocument(
            const json& document)
        {
            try
            {
                json_validator validator;

                validator.set_root_schema(
                    embeddedSchema());

                validator.validate(
                    document);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Project schema validation failed: " << e.what() << std::endl;
                throw std::runtime_error(
                    std::string(
                        "OpticForge project schema validation failed: ") +
                    e.what());
             
            }
        }

        std::string pathForMessage(
            const std::filesystem::path& path)
        {
            return path.string();
        }

    } // namespace

    telescope::TelescopeProject ProjectIO::load(
        const std::filesystem::path& path)
    {
        std::ifstream input(
            path,
            std::ios::binary);

        if (!input)
        {
            throw std::runtime_error(
                "Unable to open OpticForge project '" +
                pathForMessage(path) +
                "' for reading.");
        }

        json document;

        try
        {
            input >> document;
        }
        catch (const json::parse_error& e)
        {
            throw std::runtime_error(
                "Unable to parse OpticForge project '" +
                pathForMessage(path) +
                "': " +
                e.what());
        }

        validateDocument(document);

        try
        {
            // Deserialize into a new TelescopeProject. The caller can move-assign
            // this into the live project only after the whole operation succeeds.
            return ProjectSerializer::deserialize(
                document);
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(
                "Unable to deserialize OpticForge project '" +
                pathForMessage(path) +
                "': " +
                e.what());
        }
    }

    void ProjectIO::save(
        const telescope::TelescopeProject& project,
        const std::filesystem::path& path)
    {
        json document;

        try
        {
            document =
                ProjectSerializer::serialize(
                    project);
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(
                "Unable to serialize OpticForge project '" +
                pathForMessage(path) +
                "': " +
                e.what());
        }

        // Validate our own output too. This turns a serializer/schema mismatch
        // into an immediate development error instead of writing a file that
        // OpticForge itself cannot load later.
        validateDocument(document);

        std::ofstream output(
            path,
            std::ios::binary |
            std::ios::trunc);

        if (!output)
        {
            throw std::runtime_error(
                "Unable to open OpticForge project '" +
                pathForMessage(path) +
                "' for writing.");
        }

        output
            << document.dump(4)
            << '\n';

        if (!output)
        {
            throw std::runtime_error(
                "Failed while writing OpticForge project '" +
                pathForMessage(path) +
                "'.");
        }
    }

} // namespace opticforge::project
