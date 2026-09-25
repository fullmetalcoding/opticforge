// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include "ShaderProgram.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

class ShaderManager
{
public:
    ShaderManager() = default;

    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;

    ShaderProgram& load(
        const std::string& name,
        const std::filesystem::path& vertexShader,
        const std::filesystem::path& fragmentShader);

    ShaderProgram& load(
        const std::string& name,
        const std::filesystem::path& vertexShader,
        const std::filesystem::path& geometryShader,
        const std::filesystem::path& fragmentShader);

    ShaderProgram& get(const std::string& name);

    const ShaderProgram& get(
        const std::string& name) const;

    [[nodiscard]]
    bool contains(const std::string& name) const;

    void reload(const std::string& name);
    void reloadAll();

    void remove(const std::string& name);
    void clear();

private:
    std::unordered_map<
        std::string,
        std::unique_ptr<ShaderProgram>>
        m_programs;
};