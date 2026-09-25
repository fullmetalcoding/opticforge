// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <string>
#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>

class ShaderProgram
{
public:
    ShaderProgram() = default;

    ShaderProgram(
        const std::filesystem::path& vertexShader,
        const std::filesystem::path& fragmentShader);

    ShaderProgram(
        const std::filesystem::path& vertexShader,
        const std::filesystem::path& geometryShader,
        const std::filesystem::path& fragmentShader);

    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    void bind() const;
    static void unbind();

    [[nodiscard]]
    GLuint id() const noexcept
    {
        return m_program;
    }

    [[nodiscard]]
    bool valid() const noexcept
    {
        return m_program != 0;
    }

    void reload();

    GLint uniformLocation(const std::string& name) const;

    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec3(
        const std::string& name,
        const glm::vec3& value) const;
    void setVec2(
        const std::string& name,
        const glm::vec2& value) const;

    void setMat4(
        const std::string& name,
        const glm::mat4& value) const;

private:
    GLuint m_program = 0;

    std::filesystem::path m_vertexPath;
    std::filesystem::path m_geometryPath;
    std::filesystem::path m_fragmentPath;

    static std::string loadFile(const std::filesystem::path& path);

    static GLuint compileShader(
        GLenum shaderType,
        const std::string& source,
        const std::filesystem::path& path);

    static GLuint linkProgram(
        GLuint vertexShader,
        GLuint geometryShader,
        GLuint fragmentShader);

    void build();

    void destroy() noexcept;
};