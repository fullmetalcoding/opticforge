#pragma once

#include <string_view>

#include <nlohmann/json_fwd.hpp>

#include "telescope/TelescopeProject.h"

namespace opticforge::project
{

    class ProjectSerializer
    {
    public:
        static constexpr int FormatVersion = 1;
        static constexpr std::string_view FormatName = "OpticForgeProject";

        // Converts a complete TelescopeProject to/from the versioned on-disk
        // document representation. Schema validation is performed by ProjectIO.
        static nlohmann::json serialize(
            const telescope::TelescopeProject& project);

        static telescope::TelescopeProject deserialize(
            const nlohmann::json& document);
    };

} // namespace opticforge::project
