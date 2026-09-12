#include "ShaderManager.h"

#include <stdexcept>

ShaderProgram& ShaderManager::load(
    const std::string& name,
    const std::filesystem::path& vertexShader,
    const std::filesystem::path& fragmentShader)
{
    if (contains(name))
    {
        throw std::runtime_error(
            "Shader program already exists: " + name);
    }

    auto program =
        std::make_unique<ShaderProgram>(
            vertexShader,
            fragmentShader);

    ShaderProgram& reference = *program;

    m_programs.emplace(
        name,
        std::move(program));

    return reference;
}

ShaderProgram& ShaderManager::load(
    const std::string& name,
    const std::filesystem::path& vertexShader,
    const std::filesystem::path& geometryShader,
    const std::filesystem::path& fragmentShader)
{
    if (contains(name))
    {
        throw std::runtime_error(
            "Shader program already exists: " + name);
    }

    auto program =
        std::make_unique<ShaderProgram>(
            vertexShader,
            geometryShader,
            fragmentShader);

    ShaderProgram& reference = *program;

    m_programs.emplace(
        name,
        std::move(program));

    return reference;
}

ShaderProgram& ShaderManager::get(
    const std::string& name)
{
    const auto it = m_programs.find(name);

    if (it == m_programs.end())
    {
        throw std::runtime_error(
            "Unknown shader program: " + name);
    }

    return *it->second;
}

const ShaderProgram& ShaderManager::get(
    const std::string& name) const
{
    const auto it = m_programs.find(name);

    if (it == m_programs.end())
    {
        throw std::runtime_error(
            "Unknown shader program: " + name);
    }

    return *it->second;
}

bool ShaderManager::contains(
    const std::string& name) const
{
    return m_programs.find(name) !=
        m_programs.end();
}

void ShaderManager::reload(
    const std::string& name)
{
    get(name).reload();
}

void ShaderManager::reloadAll()
{
    for (auto& [name, program] : m_programs)
    {
        program->reload();
    }
}

void ShaderManager::remove(
    const std::string& name)
{
    m_programs.erase(name);
}

void ShaderManager::clear()
{
    m_programs.clear();
}