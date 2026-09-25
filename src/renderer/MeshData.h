// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once
#include "glm/glm.hpp"

namespace opticforge {
    struct MeshVertex
    {
        glm::vec3 position{ 0.0f };
        glm::vec3 normal{ 0.0f, 0.0f, 1.0f };
    };

    struct MeshData
    {
        std::vector<MeshVertex> vertices;
        std::vector<std::uint32_t> indices;
    };
}