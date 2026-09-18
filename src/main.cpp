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
#include "renderer/PsfTextureRenderer.h"
#include "renderer/RayPathRenderer.h"
#include "raytracer/TraceController.h"
#include "telescope/TelescopeProject.h"
#include "ui/UI.h"
#include "project/ProjectController.h"

#include <iostream>
using namespace std;

static void fatal(const char* what) {
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: %s", what, SDL_GetError());
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OpticForge", what, nullptr);
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


	// ------------------------------------------------------------
	// Telescope project
	// ------------------------------------------------------------
	opticforge::telescope::TelescopeProject project;
	// Persistent application state:
	std::uint64_t displayedTraceVersion = 0;

	opticforge::raytracer::TraceController traceController;
	opticforge::project::ProjectController projectController(
		window,
		project,
		traceController);
	opticforge::ui::ProjectCommands projectCommands
	{
		[&projectController]()
		{
			projectController.newProject();
		},

		[&projectController]()
		{
			projectController.openProject();
		},

		[&projectController]()
		{
			projectController.saveProject();
		},

		[&projectController]()
		{
			projectController.saveProjectAs();
		}
	};

	opticforge::raytracer::TraceSettings traceSettings;
	opticforge::renderer::PsfTextureRenderer psfRenderer;
	opticforge::renderer::RayPathRenderer rayPathRenderer;

	std::uint64_t displayedRayTraceVersion = 0;
	std::uint64_t displayedRayGeometryVersion = 0;

	std::uint64_t displayedPsfTraceVersion = 0;
	std::uint64_t displayedPsfSettingsVersion = 0;

	// ------------------------------------------------------------
	// Render system
	// ------------------------------------------------------------

	int width = 0;
	int height = 0;

	SDL_GetWindowSizeInPixels(
		window,
		&width,
		&height);
	opticforge::RenderSystem renderSys((float)width / (float)height);

	// ------------------------------------------------------------
	// Camera
	// ------------------------------------------------------------

	Camera camera(
		glm::vec3(
			2.0f,
			4.0 / 3.0f,
			2.0f),

		glm::vec3(
			0.0f,
			0.0f,
			0.0f));

	camera.setPerspective(
		45.0f,
		0.01f,
		1000.0f);

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
			ImGuiIO& io = ImGui::GetIO();

			switch (ev.type) {
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			{
				if (io.WantCaptureMouse)
					break;
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
				if (io.WantCaptureMouse)
					break;
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
				if (io.WantCaptureMouse)
					break;
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
		main_ui.drawUI(project, bQuit, traceController, traceSettings, projectCommands);
		// Each frame:
		traceController.setResultsNeeded(
			main_ui.showPsf() || main_ui.showRays());

		traceController.update(project, traceSettings);
		if (main_ui.showRays() &&
			(traceController.resultVersion() != displayedRayTraceVersion ||
				main_ui.rayPathGeometryVersion() != displayedRayGeometryVersion))
		{
			if (const auto* completed = traceController.latestResult())
			{
				rayPathRenderer.setTraceResult(
					completed->result,
					main_ui.rayPathSettings());

				main_ui.setRayPathStats(
					rayPathRenderer.displayedRays(),
					rayPathRenderer.truncated());

				displayedRayTraceVersion =
					traceController.resultVersion();

				displayedRayGeometryVersion =
					main_ui.rayPathGeometryVersion();
			}
		}

		if (
			main_ui.showPsf() &&
			(
				traceController.resultVersion() !=
				displayedPsfTraceVersion ||
				main_ui.psfSettingsVersion() !=
				displayedPsfSettingsVersion
				))
		{
			if (const auto* completed = traceController.latestResult())
			{
				psfRenderer.render(
					completed->result,
					completed->observationPlane,
					main_ui.psfSettings());

				main_ui.setPsfTraceTexture(
					psfRenderer.texture(),
					psfRenderer.width(),
					psfRenderer.height());

				displayedPsfTraceVersion =
					traceController.resultVersion();

				displayedPsfSettingsVersion =
					main_ui.psfSettingsVersion();
			}
		}
		ImGui::Render();

		// --------------------------------------------------------
		// Drawable dimensions
		// --------------------------------------------------------

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

		renderSys.setAspectRatio(aspect);
		renderSys.drawProject(project, camera);
		if (main_ui.showRays())
		{
			rayPathRenderer.draw(
				view,
				projection,
				main_ui.rayPathSettings());
		}



		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();
	rayPathRenderer.release();
	psfRenderer.release();
	SDL_GL_DestroyContext(gl);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
