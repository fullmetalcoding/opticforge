#include "ProjectController.h"

#include <memory>
#include <string>
#include <utility>

#include "ProjectIO.h"

namespace opticforge::project
{

    namespace
    {

        constexpr SDL_DialogFileFilter projectFilters[] =
        {
            {
                "OpticForge Project",
                "ofp"
            }
        };


        std::filesystem::path ensureProjectExtension(
            std::filesystem::path path)
        {
            if (!path.has_extension())
            {
                path += ".ofp";
            }

            return path;
        }

    }


    ProjectController::ProjectController(
        SDL_Window* window,
        telescope::TelescopeProject& project,
        raytracer::TraceController& traceController,
        ProjectReplacedCallback onProjectReplaced)
        :
        m_window(window),
        m_project(project),
        m_traceController(traceController),
        m_onProjectReplaced(std::move(onProjectReplaced))
    {
    }


    void ProjectController::newProject()
    {
        m_project.clearProject();

        m_currentPath.reset();

        m_traceController.invalidate();
        if (m_onProjectReplaced)
        {
            m_onProjectReplaced();
        }
    }


    void ProjectController::openProject()
    {
        //
        // Must be invoked from the SDL/main thread.
        //
        SDL_ShowOpenFileDialog(
            &ProjectController::openDialogCallback,
            this,
            m_window,
            projectFilters,
            1,
            nullptr,
            false);
    }


    void ProjectController::saveProject()
    {
        //
        // If this project already has a filename, Save means
        // overwrite that file without showing another dialog.
        //
        if (m_currentPath)
        {
            savePath(*m_currentPath);
            return;
        }

        //
        // An unsaved project behaves like Save As.
        //
        saveProjectAs();
    }


    void ProjectController::saveProjectAs()
    {
        SDL_ShowSaveFileDialog(
            &ProjectController::saveDialogCallback,
            this,
            m_window,
            projectFilters,
            1,
            nullptr);
    }


    void SDLCALL ProjectController::openDialogCallback(
        void* userdata,
        const char* const* filelist,
        int)
    {
        auto* controller =
            static_cast<ProjectController*>(userdata);

        //
        // nullptr means the dialog itself failed.
        //
        if (!filelist)
        {
            const std::string error =
                SDL_GetError();

            controller->dispatchToMainThread(
                [controller, error]()
                {
                    controller->showError(
                        "Open Project",
                        error);
                });

            return;
        }

        //
        // Empty list means Cancel.
        //
        if (!filelist[0])
        {
            return;
        }

        //
        // SDL paths are UTF-8.
        //
        const std::filesystem::path path =
            std::filesystem::u8path(
                filelist[0]);

        //
        // The SDL file dialog callback is permitted to execute
        // on another thread. Do not replace TelescopeProject here.
        //
        controller->dispatchToMainThread(
            [controller, path]()
            {
                controller->openPath(path);
            });
    }


    void SDLCALL ProjectController::saveDialogCallback(
        void* userdata,
        const char* const* filelist,
        int)
    {
        auto* controller =
            static_cast<ProjectController*>(userdata);

        if (!filelist)
        {
            const std::string error =
                SDL_GetError();

            controller->dispatchToMainThread(
                [controller, error]()
                {
                    controller->showError(
                        "Save Project",
                        error);
                });

            return;
        }

        //
        // User cancelled.
        //
        if (!filelist[0])
        {
            return;
        }

        std::filesystem::path path =
            std::filesystem::u8path(
                filelist[0]);

        path = ensureProjectExtension(
            std::move(path));

        controller->dispatchToMainThread(
            [controller, path]()
            {
                controller->savePath(path);
            });
    }


    void ProjectController::openPath(
        const std::filesystem::path& path)
    {
        try
        {
            //
            // Load into a separate object first.
            //
            // The live TelescopeProject remains untouched if parsing,
            // schema validation, or deserialization throws.
            //
            telescope::TelescopeProject loaded =
                ProjectIO::load(path);

            m_project =
                std::move(loaded);

            m_currentPath =
                path;

            //
            // Force the raytracer to discard results from the
            // previous project.
            //
            m_traceController.invalidate();
            if (m_onProjectReplaced)
            {
                m_onProjectReplaced();
            }
        }
        catch (const std::exception& e)
        {
            showError(
                "Could not open project",
                e.what());
        }
    }


    void ProjectController::savePath(
        const std::filesystem::path& path)
    {
        try
        {
            ProjectIO::save(
                m_project,
                path);

            //
            // Save As establishes a new current filename.
            //
            m_currentPath =
                path;
        }
        catch (const std::exception& e)
        {
            showError(
                "Could not save project",
                e.what());
        }
    }


    void ProjectController::showError(
        const char* title,
        const std::string& message) const
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            title,
            message.c_str(),
            m_window);
    }


    void ProjectController::dispatchToMainThread(
        std::function<void()> function)
    {
        auto* task =
            new MainThreadTask{
                std::move(function)
        };

        if (!SDL_RunOnMainThread(
            &ProjectController::mainThreadTaskCallback,
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


    void SDLCALL ProjectController::mainThreadTaskCallback(
        void* userdata)
    {
        std::unique_ptr<MainThreadTask> task(
            static_cast<MainThreadTask*>(
                userdata));

        task->function();
    }

}