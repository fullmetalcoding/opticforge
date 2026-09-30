// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "MaterialLibrary.h"

#include <algorithm>
#include <utility>

namespace opticforge::optics
{
    Material* MaterialLibrary::find(
        std::string_view key) noexcept
    {
        const auto it =
            std::find_if(
                m_materials.begin(),
                m_materials.end(),
                [key](const Material& material)
                {
                    return material.key == key;
                });

        return
            it == m_materials.end()
            ? nullptr
            : &*it;
    }

    const Material* MaterialLibrary::find(
        std::string_view key) const noexcept
    {
        const auto it =
            std::find_if(
                m_materials.begin(),
                m_materials.end(),
                [key](const Material& material)
                {
                    return material.key == key;
                });

        return
            it == m_materials.end()
            ? nullptr
            : &*it;
    }

    bool MaterialLibrary::contains(
        std::string_view key) const noexcept
    {
        return find(key) != nullptr;
    }

    bool MaterialLibrary::add(
        Material material)
    {
        if (
            material.key.empty() ||
            contains(material.key))
        {
            return false;
        }

        m_materials.push_back(
            std::move(material));

        return true;
    }

    bool MaterialLibrary::update(
        std::string_view originalKey,
        Material material)
    {
        Material* existing =
            find(originalKey);

        if (!existing || material.key.empty())
            return false;

        if (
            material.key != originalKey &&
            contains(material.key))
        {
            return false;
        }

        *existing =
            std::move(material);

        return true;
    }

    bool MaterialLibrary::remove(
        std::string_view key)
    {
        const auto it =
            std::remove_if(
                m_materials.begin(),
                m_materials.end(),
                [key](const Material& material)
                {
                    return material.key == key;
                });

        if (it == m_materials.end())
            return false;

        m_materials.erase(
            it,
            m_materials.end());

        return true;
    }

    void MaterialLibrary::clear()
    {
        m_metadata =
            MaterialLibraryMetadata{};

        m_materials.clear();
    }

} // namespace opticforge::optics
