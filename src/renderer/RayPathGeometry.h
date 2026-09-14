#pragma once
#include <cstddef>
#include <vector>
#include <glm/glm.hpp>
#include "raytracer/TraceResult.h"

namespace opticforge::renderer
{
    struct RayPathRenderSettings
    {
        int maxRays = 2000;
        glm::vec4 observationColor{ 0.15f, 0.8f, 1.0f, 0.06f };
        glm::vec4 escapedColor{ 1.0f, 0.35f, 0.1f, 0.04f };
        glm::vec4 terminatedColor{ 0.8f, 0.8f, 0.8f, 0.04f };
        float lineWidth = 1.0f;
        double escapeLengthMm = 1000.0;
        bool depthTest = true;
    };

    struct RayPathVertex
    {
        glm::vec3 position; // World metres, matching the scene renderer.
        float category;     // 0 observation plane, 1 escaped, 2 other termination.
    };

    struct RayPathGeometry
    {
        std::vector<RayPathVertex> vertices; // Pairs for GL_LINES.
        std::size_t displayedRays = 0;
        bool truncated = false;
    };

    // Pure CPU conversion, independent of GL. Uses a deterministic, evenly
    // spaced subset of the solution and never joins separate paths together.
    RayPathGeometry buildRayPathGeometry(const raytracer::TraceResult& result,
        const RayPathRenderSettings& settings,
        std::size_t maxSegments = 1000000);
}
