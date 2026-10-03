// Part of OpticForge.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "MaterialLibraryController.h"

#include <memory>
#include <string>
#include <utility>

#include "MaterialLibraryIO.h"

namespace opticforge::project
{
    namespace
    {
        constexpr SDL_DialogFileFilter materialFilters[] =
        {
            {
                "OpticForge Material Library",
                "ofmat"
            }
        };

        std::filesystem::path ensureMaterialExtension(
            std::filesystem::path path)
        {
            if (!path.has_extension())
                path += ".ofmat";

            return path;
        }
    }

    MaterialLibraryController::MaterialLibraryController(
        SDL_Window* window,
        optics::MaterialLibrary& library,
        LibraryReplacedCallback onLibraryReplaced)
        :
        m_window(window),
        m_library(library),
        m_onLibraryReplaced(
            std::move(onLibraryReplaced))
    {
    }

    void MaterialLibraryController::loadLibrary()
    {
        SDL_ShowOpenFileDialog(
            &MaterialLibraryController::openDialogCallback,
            this,
            m_window,
            materialFilters,
            1,
            nullptr,
            false);
    }

    void MaterialLibraryController::saveLibrary()
    {
        if (m_currentPath)
        {
            savePath(*m_currentPath);
            return;
        }

        saveLibraryAs();
    }

    void MaterialLibraryController::saveLibraryAs()
    {
        SDL_ShowSaveFileDialog(
            &MaterialLibraryController::saveDialogCallback,
            this,
            m_window,
            materialFilters,
            1,
            nullptr);
    }

    void SDLCALL MaterialLibraryController::openDialogCallback(
        void* userdata,
        const char* const* filelist,
        int)
    {
        auto* controller =
            static_cast<MaterialLibraryController*>(
                userdata);

        if (!filelist)
        {
            const std::string error =
                SDL_GetError();

            controller->dispatchToMainThread(
                [controller, error]()
                {
                    controller->showError(
                        "Load Material Library",
                        error);
                });

            return;
        }

        if (!filelist[0])
            return;

        const std::filesystem::path path =
#if defined(__cpp_char8_t)
            std::filesystem::path(
                reinterpret_cast<const char8_t*>(
                    filelist[0]));
#else
            std::filesystem::u8path(
                filelist[0]);
#endif

        controller->dispatchToMainThread(
            [controller, path]()
            {
                controller->openPath(path);
            });
    }

    void SDLCALL MaterialLibraryController::saveDialogCallback(
        void* userdata,
        const char* const* filelist,
        int)
    {
        auto* controller =
            static_cast<MaterialLibraryController*>(
                userdata);

        if (!filelist)
        {
            const std::string error =
                SDL_GetError();

            controller->dispatchToMainThread(
                [controller, error]()
                {
                    controller->showError(
                        "Save Material Library",
                        error);
                });

            return;
        }

        if (!filelist[0])
            return;

        std::filesystem::path path =
#if defined(__cpp_char8_t)
            std::filesystem::path(
                reinterpret_cast<const char8_t*>(
                    filelist[0]));
#else
            std::filesystem::u8path(
                filelist[0]);
#endif

        path =
            ensureMaterialExtension(
                std::move(path));

        controller->dispatchToMainThread(
            [controller, path]()
            {
                controller->savePath(path);
            });
    }

    void MaterialLibraryController::openPath(
        const std::filesystem::path& path)
    {
        try
        {
            optics::MaterialLibrary loaded =
                MaterialLibraryIO::load(
                    path);

            m_library =
                std::move(loaded);

            m_currentPath =
                path;

            if (m_onLibraryReplaced)
                m_onLibraryReplaced();
        }
        catch (const std::exception& e)
        {
            showError(
                "Could not load material library",
                e.what());
        }
    }

    void MaterialLibraryController::savePath(
        const std::filesystem::path& path)
    {
        try
        {
            MaterialLibraryIO::save(
                m_library,
                path);

            m_currentPath =
                path;
        }
        catch (const std::exception& e)
        {
            showError(
                "Could not save material library",
                e.what());
        }
    }

    void MaterialLibraryController::showError(
        const char* title,
        const std::string& message) const
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            title,
            message.c_str(),
            m_window);
    }

    void MaterialLibraryController::dispatchToMainThread(
        std::function<void()> function)
    {
        auto* task =
            new MainThreadTask{
                std::move(function)
            };

        if (!SDL_RunOnMainThread(
            &MaterialLibraryController::mainThreadTaskCallback,
            task,
            false))
        {
            delete task;

            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "SDL_RunOnMainThread failed: %s",
                SDL_GetError());
        }
    }

    void SDLCALL MaterialLibraryController::mainThreadTaskCallback(
        void* userdata)
    {
        std::unique_ptr<MainThreadTask> task(
            static_cast<MainThreadTask*>(
                userdata));

        task->function();
    }

} // namespace opticforge::project
