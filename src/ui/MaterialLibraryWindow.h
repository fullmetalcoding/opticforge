// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <functional>
#include <optional>
#include <string>

#include "optics/MaterialLibrary.h"

namespace opticforge::ui
{
    struct MaterialLibraryCommands
    {
        std::function<void()> load;
        std::function<void()> save;
        std::function<void()> saveAs;
        std::function<void()> changed;
    };

    class MaterialLibraryWindow
    {
    public:
        void open() noexcept
        {
            m_visible = true;
        }

        void notifyLibraryReplaced()
        {
            m_selectedKey.clear();
            m_editing = false;
            m_originalKey.reset();
        }

        void draw(
            optics::MaterialLibrary& library,
            const MaterialLibraryCommands& commands);

    private:
        bool m_visible = false;
        std::string m_selectedKey;

        bool m_editing = false;
        std::optional<std::string> m_originalKey;
        optics::Material m_editMaterial;

        std::string m_error;

        void beginNew(
            const optics::MaterialLibrary& library);

        void beginEdit(
            const optics::MaterialLibrary& library);

        void drawEditor(
            optics::MaterialLibrary& library,
            const MaterialLibraryCommands& commands);

        static void drawRefractiveIndexModel(
            const char* id,
            optics::RefractiveIndexModel& model);

        static bool validMaterial(
            const optics::Material& material);
    };
}
