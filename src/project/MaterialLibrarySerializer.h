// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <nlohmann/json_fwd.hpp>

#include "optics/MaterialLibrary.h"

namespace opticforge::project
{
    class MaterialLibrarySerializer
    {
    public:
        static constexpr int FormatVersion = 1;

        static nlohmann::json serialize(
            const optics::MaterialLibrary& library);

        static optics::MaterialLibrary deserialize(
            const nlohmann::json& document);
    };
}
