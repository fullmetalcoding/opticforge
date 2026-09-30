// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <functional>
#include <optional>

#include <SDL3/SDL.h>

#include "optics/MaterialLibrary.h"

namespace opticforge::project
{
    class MaterialLibraryController
    {
    public:
        using LibraryReplacedCallback =
            std::function<void()>;

        MaterialLibraryController(
            SDL_Window* window,
            optics::MaterialLibrary& library,
            LibraryReplacedCallback onLibraryReplaced = {});

        void loadLibrary();
        void saveLibrary();
        void saveLibraryAs();

        const std::optional<std::filesystem::path>&
            currentPath() const noexcept
        {
            return m_currentPath;
        }

    private:
        SDL_Window* m_window = nullptr;
        optics::MaterialLibrary& m_library;
        LibraryReplacedCallback m_onLibraryReplaced;

        std::optional<std::filesystem::path>
            m_currentPath;

        static void SDLCALL openDialogCallback(
            void* userdata,
            const char* const* filelist,
            int filter);

        static void SDLCALL saveDialogCallback(
            void* userdata,
            const char* const* filelist,
            int filter);

        void openPath(
            const std::filesystem::path& path);

        void savePath(
            const std::filesystem::path& path);

        void showError(
            const char* title,
            const std::string& message) const;

        void dispatchToMainThread(
            std::function<void()> function);

        struct MainThreadTask
        {
            std::function<void()> function;
        };

        static void SDLCALL mainThreadTaskCallback(
            void* userdata);
    };
}
