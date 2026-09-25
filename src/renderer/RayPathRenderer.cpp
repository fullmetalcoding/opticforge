// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "RayPathRenderer.h"
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace opticforge::renderer
{
    void RayPathRenderer::setTraceResult(const raytracer::TraceResult& result,
        const RayPathRenderSettings& settings)
    {
        const auto geometry = buildRayPathGeometry(result, settings);
        if (!m_shader.valid())
            m_shader = ShaderProgram("shaders/raypath.vert", "shaders/raypath.frag");

        GLint vao = 0, buffer = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
        if (!m_vao) glGenVertexArrays(1, &m_vao);
        if (!m_vbo) glGenBuffers(1, &m_vbo);
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(geometry.vertices.size() * sizeof(RayPathVertex)),
            geometry.vertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RayPathVertex),
            reinterpret_cast<void*>(offsetof(RayPathVertex, position)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(RayPathVertex),
            reinterpret_cast<void*>(offsetof(RayPathVertex, category)));
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        m_vertexCount = static_cast<GLsizei>(geometry.vertices.size());
        m_displayedRays = geometry.displayedRays;
        m_truncated = geometry.truncated;
    }

    void RayPathRenderer::draw(const glm::mat4& view, const glm::mat4& projection,
        const RayPathRenderSettings& settings)
    {
        if (!m_vertexCount || !m_shader.valid()) return;
        GLint program = 0, vao = 0, srcRgb = 0, dstRgb = 0, srcAlpha = 0, dstAlpha = 0;
        GLint equationRgb = 0, equationAlpha = 0, depthFunc = 0;
        GLboolean depthMask = GL_TRUE;
        GLfloat lineWidth = 1.0f, lineRange[2];
        const GLboolean blend = glIsEnabled(GL_BLEND);
        const GLboolean depth = glIsEnabled(GL_DEPTH_TEST);
        const GLboolean smooth = glIsEnabled(GL_LINE_SMOOTH);
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
        glGetIntegerv(GL_BLEND_SRC_RGB, &srcRgb);
        glGetIntegerv(GL_BLEND_DST_RGB, &dstRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
        glGetIntegerv(GL_BLEND_EQUATION_RGB, &equationRgb);
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &equationAlpha);
        glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
        glGetFloatv(GL_LINE_WIDTH, &lineWidth);
        glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineRange);

        glEnable(GL_BLEND);
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        // Alpha weights the light added by each line. Overlaps become brighter;
        // preserve framebuffer alpha and never let one ray occlude another.
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
        if (settings.depthTest) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glDisable(GL_LINE_SMOOTH);
        const float requested = std::isfinite(settings.lineWidth) ? settings.lineWidth : 1.0f;
        glLineWidth(std::clamp(requested, lineRange[0], lineRange[1]));
        m_shader.bind();
        m_shader.setMat4("uView", view);
        m_shader.setMat4("uProjection", projection);
        const auto color = [&](const char* name, const glm::vec4& value)
            {
                glUniform4f(m_shader.uniformLocation(name), value.r, value.g, value.b, value.a);
            };
        color("uObservationColor", settings.observationColor);
        color("uEscapedColor", settings.escapedColor);
        color("uTerminatedColor", settings.terminatedColor);
        glBindVertexArray(m_vao);
        glDrawArrays(GL_LINES, 0, m_vertexCount);

        glBindVertexArray(vao);
        glUseProgram(program);
        glLineWidth(lineWidth);
        if (smooth) glEnable(GL_LINE_SMOOTH);
        glDepthMask(depthMask);
        glDepthFunc(depthFunc);
        if (depth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        glBlendEquationSeparate(equationRgb, equationAlpha);
        glBlendFuncSeparate(srcRgb, dstRgb, srcAlpha, dstAlpha);
        if (!blend) glDisable(GL_BLEND);
    }

    void RayPathRenderer::release()
    {
        if (m_vbo) glDeleteBuffers(1, &m_vbo);
        if (m_vao) glDeleteVertexArrays(1, &m_vao);
        m_vbo = m_vao = 0;
        m_vertexCount = 0;
        m_displayedRays = 0;
        m_truncated = false;
        m_shader = ShaderProgram{};
    }
}
    