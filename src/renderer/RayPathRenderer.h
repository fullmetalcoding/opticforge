// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once
#include "ShaderProgram.h"
#include "RayPathGeometry.h"

namespace opticforge::renderer
{
    class RayPathRenderer
    {
    public:
        RayPathRenderer() = default;
        RayPathRenderer(const RayPathRenderer&) = delete;
        RayPathRenderer& operator=(const RayPathRenderer&) = delete;
        // All methods require the application's current GL context.
        void setTraceResult(const raytracer::TraceResult& result,
            const RayPathRenderSettings& settings);
        void draw(const glm::mat4& view, const glm::mat4& projection,
            const RayPathRenderSettings& settings);
        // Explicitly release before destroying the GL context.
        void release();
        std::size_t displayedRays() const { return m_displayedRays; }
        bool truncated() const { return m_truncated; }

    private:
        ShaderProgram m_shader;
        GLuint m_vao = 0;
        GLuint m_vbo = 0;
        GLsizei m_vertexCount = 0;
        std::size_t m_displayedRays = 0;
        bool m_truncated = false;
    };
}
