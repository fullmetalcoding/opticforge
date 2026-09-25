// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>

#include "telescope/TelescopeProject.h"

namespace opticforge::project
{

    class ProjectIO
    {
    public:
        static telescope::TelescopeProject load(
            const std::filesystem::path& path);

        static void save(
            const telescope::TelescopeProject& project,
            const std::filesystem::path& path);
    };

} // namespace opticforge::project
