// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>

#include "optics/MaterialLibrary.h"

namespace opticforge::project
{
    class MaterialLibraryIO
    {
    public:
        static optics::MaterialLibrary load(
            const std::filesystem::path& path);

        static void save(
            const optics::MaterialLibrary& library,
            const std::filesystem::path& path);
    };
}
