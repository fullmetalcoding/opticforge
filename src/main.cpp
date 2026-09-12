// opticforge.cpp : Defines the entry point for the application.
//


#include <SDL3/SDL.h>
#include <GL/glew.h>          // must be included before any GL headers

#include "imgui.h"
#include "implot.h"
#include "imgui_internal.h"   // BeginViewportSideBar -- see note at StatusBar()
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_opengl3.h"
#include "renderer/camera.h"
#include "renderer/OrbitCameraController.h"
#include "renderer/ShaderManager.h"
#include "renderer/RenderSystem.h"
#include "telescope/TelescopeProject.h"
#include "ui/UI.h"

#include <iostream>
using namespace std;

static void fatal(const char* what) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: %s", what, SDL_GetError());
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OpticForge", what, nullptr);
}

uint64_t addDefaultLens(opticforge::telescope::TelescopeProject & project) {
    using namespace opticforge;

    // Lens parameters, all linear dimensions in mm.
    constexpr double diameter = 200.0;
    constexpr double focalLength = 1600.0;
    constexpr double thickness = 20.0;

    // Approximate BK7 refractive index near the d-line.
    constexpr double nAir = 1.0;
    constexpr double nGlass = 1.5168;

    // Plano-convex radius from the thin-lens approximation:
    //
    //     1/f = (n - 1) / R
    //
    constexpr double radiusOfCurvature =
        (nGlass - 1.0) * focalLength;


    // -----------------------------------------------------------------------------
    // Front surface: convex conic
    // -----------------------------------------------------------------------------

    optics::OpticalSurface frontSurface;

    frontSurface.setGeometry(
        optics::ConicGeometry{
            radiusOfCurvature,
            0.0                    // k = 0 -> sphere
        });

    frontSurface.setAperture(
        optics::Aperture{
        optics::CircularAperture{
            diameter * 0.5
        } });

    frontSurface.setOpticalInterface(
        optics::OpticalInterface{
        optics::RefractiveInterface{
            nAir,
            nGlass
        } });


    // -----------------------------------------------------------------------------
    // Rear surface: plane
    // -----------------------------------------------------------------------------

    optics::OpticalSurface rearSurface;

    rearSurface.setGeometry(
        optics::PlaneGeometry{});

    rearSurface.setAperture(
        optics::Aperture{
        optics::CircularAperture{
            diameter * 0.5
        } });

    rearSurface.setOpticalInterface(
        optics::OpticalInterface{
        optics::RefractiveInterface{
            nGlass,
            nAir
        } });


    // -----------------------------------------------------------------------------
    // Lens primitive
    // -----------------------------------------------------------------------------

    telescope::Lens lens;

    lens.transform =
        optics::Transform{};

    lens.frontSurface =
        frontSurface;

    lens.rearSurface =
        rearSurface;

    lens.centerThickness =
        thickness;

    lens.transform.translate({ 00.0, 250, 0.0 });

    // -----------------------------------------------------------------------------
    // Add to project
    // -----------------------------------------------------------------------------

    telescope::PrimitiveId lensId =
        project.addPrimitive(
            std::move(lens));

    return lensId;

}
int main(int, char**)
{
    std::cout << "COLOSSUS ONLINE..." << std::endl;
    if (!SDL_Init(SDL_INIT_VIDEO)) { fatal("SDL_Init failed"); return 1; }

    // Request 3.3 core -- enough for ImGui's GL3 backend, and what GLEW will
    // resolve against. Attributes must be set before context creation.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif

    const SDL_WindowFlags winFlags =
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;

    SDL_Window* window = SDL_CreateWindow("OpticForge", 1280, 800, winFlags);
    if (!window) { fatal("SDL_CreateWindow failed"); SDL_Quit(); return 1; }
    SDL_SetWindowMinimumSize(window, 800, 500);

    SDL_GLContext gl = SDL_GL_CreateContext(window);
    if (!gl) { fatal("SDL_GL_CreateContext failed"); SDL_DestroyWindow(window); SDL_Quit(); return 1; }
    SDL_GL_MakeCurrent(window, gl);
    SDL_GL_SetSwapInterval(1);   // vsync; this is a monitoring UI, not a game

    // GLEW must be initialised after a context is current. glewExperimental is
    // required for core profiles or glewInit reports functions as unavailable.
    glewExperimental = GL_TRUE;
    if (GLenum err = glewInit(); err != GLEW_OK) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "glewInit failed: %s",
            reinterpret_cast<const char*>(glewGetErrorString(err)));
        SDL_GL_DestroyContext(gl);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    // glewInit() can leave a spurious GL_INVALID_ENUM on core profiles. Clear it
    // so it does not surface later and look like our own bug.
    glGetError();

    SDL_Log("GL %s | GLSL %s | %s",
        reinterpret_cast<const char*>(glGetString(GL_VERSION)),
        reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)),
        reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    // --- Dear ImGui ----------------------------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // ImPlot keeps its own context alongside ImGui's; without this the first
    // plot call dereferences null.
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = "opticforge_layout.ini";

    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForOpenGL(window, gl);
    ImGui_ImplOpenGL3_Init("#version 330");

    // ------------------------------------------------------------
 // OpenGL state
 // ------------------------------------------------------------

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);

    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA);

    /*
     * OpenGL core profile requires a VAO to be bound even though
     * our fullscreen triangle obtains its vertices from gl_VertexID.
     */
    GLuint fullscreenVao = 0;

    glGenVertexArrays(
        1,
        &fullscreenVao);

    glBindVertexArray(
        fullscreenVao);
    GLuint emptyVao = 0;

    glGenVertexArrays(1, &emptyVao);
    glBindVertexArray(emptyVao);
    // ------------------------------------------------------------
    // Shader
    // ------------------------------------------------------------

    ShaderProgram gridShader(
        "shaders/grid.vert",
        "shaders/grid.frag");


    
    ShaderProgram planeShader(
        "shaders/plane.vert",
        "shaders/plane.frag");

    ShaderProgram planeCircShader(
        "shaders/planeCirc.vert",
        "shaders/planeCirc.frag"
    );
    // ------------------------------------------------------------
    // Telescope project
    // ------------------------------------------------------------
    opticforge::telescope::TelescopeProject project; 

  //  addDefaultLens(project);
    

    // ------------------------------------------------------------
    // Render system
    // ------------------------------------------------------------

    int width = 0;
    int height = 0;

    SDL_GetWindowSizeInPixels(
        window,
        &width,
        &height);
    opticforge::RenderSystem renderSys((float)width/(float)height); 

    // ------------------------------------------------------------
    // Camera
    // ------------------------------------------------------------

    Camera camera(
        glm::vec3(
            2.0f,
            4.0/3.0f,
            2.0f),

        glm::vec3(
            0.0f,
            0.0f,
            0.0f));

    camera.setPerspective(
        45.0f,
        0.01f,
        1000.0f);

    glm::vec3 focusCenter{ 0,.25,.5 }; //Focus plane center
    glm::vec3 focusNorm{ 0,0,-1 }; //Focus plane normal
    glm::vec3 focusCol{ 1.0, 0.0, 0.0 }; //Focus plane color
    float focusOpac{ 0.6 }; //Focus plane opacity
    glm::vec2 focusSize{ 0.025,0.025 }; //Focus plane size

    glm::vec3 pupilCenter{ 0,.25,-0.5 }; //pupil plane center
    glm::vec3 pupilNorm{ 0,0,1 }; //pupil plane normal
    glm::vec3 pupilCol{ 0.0, 0.0, 1.0 }; //pupil plane color
    float pupilOpac{ 0.6 }; //pupil plane opacity
    float pupilSize{ .25 }; //Pupil plane radius

    OrbitCameraController controller(
        camera);

    opticforge::ui::UI main_ui; 

    bool bQuit = false; 
    bool leftMouseDown = false;
    bool rightMouseDown = false;
    // --- Main loop -----------------------------------------------------------
    while (!bQuit) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL3_ProcessEvent(&ev);
            if (ev.type == SDL_EVENT_QUIT) bQuit = true;
            if (ev.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                ev.window.windowID == SDL_GetWindowID(window)) {
                bQuit = true; 
            }
            switch (ev.type) {
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                if (ev.button.button ==
                    SDL_BUTTON_LEFT)
                {
                    leftMouseDown = true;
                }

                if (ev.button.button ==
                    SDL_BUTTON_RIGHT)
                {
                    rightMouseDown = true;
                }

                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                if (ev.button.button ==
                    SDL_BUTTON_LEFT)
                {
                    leftMouseDown = false;
                }

                if (ev.button.button ==
                    SDL_BUTTON_RIGHT)
                {
                    rightMouseDown = false;
                }

                break;
            }

            case SDL_EVENT_MOUSE_MOTION:
            {
                const float dx =
                    ev.motion.xrel;

                const float dy =
                    ev.motion.yrel;

                if (leftMouseDown)
                {
                    controller.orbit(
                        dx,
                        dy);
                }

                if (rightMouseDown)
                {
                    controller.pan(
                        dx,
                        dy);
                }

                break;
            }

            case SDL_EVENT_MOUSE_WHEEL:
            {
                controller.zoom(
                    ev.wheel.y);

                break;
            }

            }
        }

        // Minimised: skip rendering but keep the event pump alive.
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }



        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        main_ui.drawUI(project, bQuit); 

        ImGui::Render();

        // --------------------------------------------------------
        // Drawable dimensions
        // --------------------------------------------------------

        int width = 0;
        int height = 0;

        SDL_GetWindowSizeInPixels(
            window,
            &width,
            &height);

        if (width <= 0 ||
            height <= 0)
        {
            continue;
        }

        glViewport(
            0,
            0,
            width,
            height);

        const float aspect =
            static_cast<float>(width) /
            static_cast<float>(height);

        // --------------------------------------------------------
        // Clear
        // --------------------------------------------------------

        glClearColor(
            0.075f,
            0.080f,
            0.090f,
            1.0f);

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);

        // --------------------------------------------------------
        // Camera matrices
        // --------------------------------------------------------

        const glm::mat4 view =
            camera.viewMatrix();

        const glm::mat4 projection =
            camera.projectionMatrix(
                aspect);

        const glm::mat4 invView =
            glm::inverse(view);

        const glm::mat4 invProjection =
            glm::inverse(projection);

        // --------------------------------------------------------
        // Ground grid
        // --------------------------------------------------------

        gridShader.bind();
        gridShader.setMat4(
            "uView",
            view);

        gridShader.setMat4(
            "uProjection",
            projection);
        gridShader.setMat4(
            "uInvView",
            invView);

        gridShader.setMat4(
            "uInvProjection",
            invProjection);

        gridShader.setFloat(
            "uMinorSpacing",
            0.25f);

        gridShader.setFloat(
            "uMajorSpacing",
            1.0f);

        gridShader.setFloat(
            "uFadeDistance",
            100.0f);

        gridShader.setFloat(
            "uOpacity",
            0.65f);

        gridShader.setVec3(
            "uMinorColor",
            glm::vec3(
                0.25f,
                0.25f,
                0.27f));

        gridShader.setVec3(
            "uMajorColor",
            glm::vec3(
                0.08f,
                0.08f,
                0.92f));

        glBindVertexArray(
            fullscreenVao);

        glDrawArrays(
            GL_TRIANGLES,
            0,
            3);

        ShaderProgram::unbind();

        planeShader.bind();

        planeShader.setMat4("uView", view);
        planeShader.setMat4("uProjection", projection);

        planeShader.setVec3("uCenter", focusCenter);
        planeShader.setVec3("uNormal", focusNorm);
        planeShader.setVec2("uSize", focusSize);

        planeShader.setVec3("uColor", focusCol);
        planeShader.setFloat("uOpacity", focusOpac);

        glBindVertexArray(emptyVao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        ShaderProgram::unbind();

        planeCircShader.bind(); 
        planeCircShader.setMat4("uView", view);
        planeCircShader.setMat4("uProjection", projection);

        planeCircShader.setVec3("uCenter", pupilCenter);
        planeCircShader.setVec3("uNormal", pupilNorm);
        planeCircShader.setFloat("uRadius", pupilSize);

        planeCircShader.setVec3("uColor", pupilCol);
        planeCircShader.setFloat("uOpacity", pupilOpac);

        glBindVertexArray(emptyVao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
        ShaderProgram::unbind();

        renderSys.drawProject(project, camera); 


        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl);
    SDL_DestroyWindow(window);
    SDL_Quit();
	return 0;
}
