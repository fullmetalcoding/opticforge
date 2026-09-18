#pragma once

#include <filesystem>
#include <functional>
#include <optional>

#include <SDL3/SDL.h>

#include "raytracer/TraceController.h"
#include "telescope/TelescopeProject.h"

namespace opticforge::project
{

    class ProjectController
    {
    public:
        using ProjectReplacedCallback =
            std::function<void()>;
        ProjectController(
            SDL_Window* window,
            telescope::TelescopeProject& project,
            raytracer::TraceController& traceController,
            ProjectReplacedCallback onProjectReplaced
            );

        void newProject();

        void openProject();

        void saveProject();

        void saveProjectAs();

        const std::optional<std::filesystem::path>&
            currentPath() const noexcept
        {
            return m_currentPath;
        }

    private:
        SDL_Window* m_window = nullptr;

        telescope::TelescopeProject& m_project;

        raytracer::TraceController& m_traceController;
        ProjectReplacedCallback m_onProjectReplaced;

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