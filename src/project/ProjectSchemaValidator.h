#pragma once
#include "EmbeddedProjectSchema.h"
#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>

namespace opticforge::project {


    class ProjectSchemaValidator
    {
    public:
        ProjectSchemaValidator()
        {
            const auto schema =
                nlohmann::json::parse(
                    opticforge::project::projectSchemaJson());

            m_validator.set_root_schema(schema);
        }

        void validate(const nlohmann::json& project) const
        {
            m_validator.validate(project);
        }

    private:
        nlohmann::json_schema::json_validator m_validator;
    };
}