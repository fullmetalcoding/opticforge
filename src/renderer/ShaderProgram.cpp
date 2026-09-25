// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "ShaderProgram.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>
#include <iostream>

namespace
{
    const char* shaderTypeName(GLenum type)
    {
        switch (type)
        {
        case GL_VERTEX_SHADER:
            return "vertex";

        case GL_FRAGMENT_SHADER:
            return "fragment";

        case GL_GEOMETRY_SHADER:
            return "geometry";

        default:
            return "unknown";
        }
    }
}

ShaderProgram::ShaderProgram(
    const std::filesystem::path& vertexShader,
    const std::filesystem::path& fragmentShader)
    : m_vertexPath(vertexShader),
    m_fragmentPath(fragmentShader)
{
    build();
}

ShaderProgram::ShaderProgram(
    const std::filesystem::path& vertexShader,
    const std::filesystem::path& geometryShader,
    const std::filesystem::path& fragmentShader)
    : m_vertexPath(vertexShader),
    m_geometryPath(geometryShader),
    m_fragmentPath(fragmentShader)
{
    build();
}

ShaderProgram::~ShaderProgram()
{
    destroy();
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : m_program(std::exchange(other.m_program, 0)),
    m_vertexPath(std::move(other.m_vertexPath)),
    m_geometryPath(std::move(other.m_geometryPath)),
    m_fragmentPath(std::move(other.m_fragmentPath))
{
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
    if (this == &other)
        return *this;

    destroy();

    m_program = std::exchange(other.m_program, 0);

    m_vertexPath = std::move(other.m_vertexPath);
    m_geometryPath = std::move(other.m_geometryPath);
    m_fragmentPath = std::move(other.m_fragmentPath);

    return *this;
}

void ShaderProgram::bind() const
{
    glUseProgram(m_program);
}

void ShaderProgram::unbind()
{
    glUseProgram(0);
}

std::string ShaderProgram::loadFile(
    const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);

    if (!file)
    {
        throw std::runtime_error(
            "Failed to open shader file: " + path.string());
    }

    std::ostringstream stream;
    stream << file.rdbuf();

    return stream.str();
}

GLuint ShaderProgram::compileShader(
    GLenum shaderType,
    const std::string& source,
    const std::filesystem::path& path)
{
    const GLuint shader = glCreateShader(shaderType);

    if (shader == 0)
    {
        throw std::runtime_error(
            "glCreateShader failed for " + path.string());
    }

    const char* sourcePtr = source.c_str();

    glShaderSource(
        shader,
        1,
        &sourcePtr,
        nullptr);

    glCompileShader(shader);

    GLint success = GL_FALSE;

    glGetShaderiv(
        shader,
        GL_COMPILE_STATUS,
        &success);

    if (success == GL_TRUE)
        return shader;

    GLint logLength = 0;

    glGetShaderiv(
        shader,
        GL_INFO_LOG_LENGTH,
        &logLength);

    std::string log;

    if (logLength > 0)
    {
        log.resize(static_cast<std::size_t>(logLength));

        glGetShaderInfoLog(
            shader,
            logLength,
            nullptr,
            log.data());
    }

    glDeleteShader(shader);

    throw std::runtime_error(
        "Failed to compile " +
        std::string(shaderTypeName(shaderType)) +
        " shader '" +
        path.string() +
        "':\n" +
        log);
}

GLuint ShaderProgram::linkProgram(
    GLuint vertexShader,
    GLuint geometryShader,
    GLuint fragmentShader)
{
    const GLuint program = glCreateProgram();

    if (program == 0)
    {
        throw std::runtime_error(
            "glCreateProgram failed");
    }

    glAttachShader(program, vertexShader);

    if (geometryShader != 0)
        glAttachShader(program, geometryShader);

    glAttachShader(program, fragmentShader);

    glLinkProgram(program);

    GLint success = GL_FALSE;

    glGetProgramiv(
        program,
        GL_LINK_STATUS,
        &success);

    if (success == GL_TRUE)
        return program;

    GLint logLength = 0;

    glGetProgramiv(
        program,
        GL_INFO_LOG_LENGTH,
        &logLength);

    std::string log;

    if (logLength > 0)
    {
        log.resize(static_cast<std::size_t>(logLength));

        glGetProgramInfoLog(
            program,
            logLength,
            nullptr,
            log.data());
    }

    glDeleteProgram(program);

    throw std::runtime_error(
        "Failed to link shader program:\n" + log);
}

void ShaderProgram::build()
{
    const std::string vertexSource =
        loadFile(m_vertexPath);

    const std::string fragmentSource =
        loadFile(m_fragmentPath);

    GLuint vertexShader = 0;
    GLuint geometryShader = 0;
    GLuint fragmentShader = 0;

    try
    {
        vertexShader = compileShader(
            GL_VERTEX_SHADER,
            vertexSource,
            m_vertexPath);

        if (!m_geometryPath.empty())
        {
            const std::string geometrySource =
                loadFile(m_geometryPath);

            geometryShader = compileShader(
                GL_GEOMETRY_SHADER,
                geometrySource,
                m_geometryPath);
        }

        fragmentShader = compileShader(
            GL_FRAGMENT_SHADER,
            fragmentSource,
            m_fragmentPath);

        const GLuint newProgram =
            linkProgram(
                vertexShader,
                geometryShader,
                fragmentShader);

        glDeleteShader(vertexShader);

        if (geometryShader != 0)
            glDeleteShader(geometryShader);

        glDeleteShader(fragmentShader);

        destroy();
        m_program = newProgram;
    }
    catch (const std::exception e)
    {
        std::cerr << "Shader compiler exception: " << e.what() << std::endl;
        if (vertexShader != 0)
            glDeleteShader(vertexShader);

        if (geometryShader != 0)
            glDeleteShader(geometryShader);

        if (fragmentShader != 0)
            glDeleteShader(fragmentShader);

        throw;
    }
}

void ShaderProgram::reload()
{
    /*
     * build() creates the new program first and only destroys the
     * existing program after the new one links successfully.
     *
     * Therefore a failed reload leaves the existing program intact.
     */
    build();
}

void ShaderProgram::destroy() noexcept
{
    if (m_program != 0)
    {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

GLint ShaderProgram::uniformLocation(
    const std::string& name) const
{
    return glGetUniformLocation(
        m_program,
        name.c_str());
}

void ShaderProgram::setBool(
    const std::string& name,
    bool value) const
{
    glUniform1i(
        uniformLocation(name),
        value ? 1 : 0);
}

void ShaderProgram::setInt(
    const std::string& name,
    int value) const
{
    glUniform1i(
        uniformLocation(name),
        value);
}

void ShaderProgram::setFloat(
    const std::string& name,
    float value) const
{
    glUniform1f(
        uniformLocation(name),
        value);
}

void ShaderProgram::setVec3(
    const std::string& name,
    const glm::vec3& value) const
{
    glUniform3fv(
        uniformLocation(name),
        1,
        &value[0]);
}

void ShaderProgram::setVec2(
    const std::string& name,
    const glm::vec2& value) const
{
    glUniform2fv(
        uniformLocation(name),
        1,
        &value[0]);
}


void ShaderProgram::setMat4(
    const std::string& name,
    const glm::mat4& value) const
{
    glUniformMatrix4fv(
        uniformLocation(name),
        1,
        GL_FALSE,
        &value[0][0]);
}