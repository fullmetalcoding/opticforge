# CMakeList.txt : CMake project for opticforge, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.21)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(OPTICFORGE_VERSION "0.1.0")
set(OPTICFORGE_VERSION_FULL "0.1.0-nogit")

project (OpticForge
   VERSION ${OPTICFORGE_VERSION}
   LANGUAGES CXX)
   
# Available to the application, so an About box or a log line can report the
# exact build rather than a number someone remembered to bump.
add_compile_definitions(OPTICFORGE_VERSION_STRING="${OPTICFORGE_VERSION_FULL}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Multi-config generators (VS) ignore CMAKE_BUILD_TYPE -- always use $<CONFIG>.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")


# -----------------------------------------------------------------------------
#  Dependency versions -- bump deliberately, not automatically.
#  Pinned tags rather than branches so a clean clone is reproducible.
# -----------------------------------------------------------------------------

set(SDL3_TAG      "release-3.2.0")
set(GLEW_TAG      "glew-cmake-2.2.0")
set(IMGUI_TAG     "v1.91.5-docking")
set(IMPLOT_TAG    "v0.16")
set(JSON_TAG      "v3.11.3")
set(GLM_TAG       "1.0.3")


include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# --- SDL3 ---------------------------------------------------------------------
# Also provides SDL_LoadObject/SDL_LoadFunction, which is how every camera
# backend reaches its vendor SDK. No separate dynamic-loading dependency.
set(SDL_SHARED   ON  CACHE BOOL "" FORCE)
set(SDL_STATIC   OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL  OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        ${SDL3_TAG}
    GIT_SHALLOW    TRUE)

# --- GLEW ---------------------------------------------------------------------
# Perlmint's fork rather than upstream: upstream's repo needs a code-generation
# step before it will build, while this one is FetchContent-friendly.
# Target is libglew_static.
set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(glew-cmake_BUILD_STATIC ON  CACHE BOOL "" FORCE)
set(ONLY_LIBS               ON  CACHE BOOL "" FORCE)
FetchContent_Declare(glew
    GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
    GIT_TAG        ${GLEW_TAG}
    GIT_SHALLOW    TRUE)

# --- Dear ImGui + ImPlot ------------------------------------------------------
# Neither ships a CMakeLists; we declare the targets ourselves below.
# Docking branch: useful for a monitoring UI with detachable panels.
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        ${IMGUI_TAG}
    GIT_SHALLOW    TRUE)

FetchContent_Declare(implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG        ${IMPLOT_TAG}
    GIT_SHALLOW    TRUE)

# --- nlohmann/json ------------------------------------------------------------
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        ${JSON_TAG}
    GIT_SHALLOW    TRUE)


# --- GLM ----------------------------------------------------------------------
# Header-only mathematics library used by the renderer/camera code.
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        ${GLM_TAG}
    GIT_SHALLOW    TRUE)

    
# CMake 4 removed compatibility with projects declaring cmake_minimum_required
# below 3.5, and glew-cmake still declares an older minimum. This is the
# sanctioned escape hatch, scoped to the dependency build so our own project
# keeps its real minimum. Unknown and harmless on CMake < 3.31.
set(_opticforge_saved_policy_min "${CMAKE_POLICY_VERSION_MINIMUM}")
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_MakeAvailable(SDL3 glew imgui implot json glm)

if(_opticforge_saved_policy_min STREQUAL "")
    unset(CMAKE_POLICY_VERSION_MINIMUM)
else()
    set(CMAKE_POLICY_VERSION_MINIMUM "${_opticforge_saved_policy_min}")
endif()

# -----------------------------------------------------------------------------
#  ImGui / ImPlot targets
# -----------------------------------------------------------------------------

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp )
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLEW)

add_library(implot STATIC
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
    ${implot_SOURCE_DIR}/implot_demo.cpp )
target_include_directories(implot PUBLIC ${implot_SOURCE_DIR})
target_link_libraries(implot PUBLIC imgui)

set(OPTICFORGE_SOURCES
    src/main.cpp
    src/renderer/ShaderProgram.cpp
    src/renderer/ShaderProgram.h
    src/renderer/ShaderManager.cpp
    src/renderer/ShaderManager.h
    src/renderer/Camera.cpp
    src/renderer/Camera.h
    src/renderer/OrbitCameraController.cpp
    src/renderer/OrbitCameraController.h
    src/renderer/RenderSystem.cpp
    src/renderer/MeshGpu.cpp
    src/renderer/LensMeshGenerator.cpp
    src/renderer/MirrorMeshGenerator.cpp
    src/optics/ConicGeometry.cpp
    src/optics/PlaneGeometry.cpp
    src/optics/Transform.cpp
    src/optics/Aperture.cpp
    src/optics/OpticalSurface.cpp
    src/optics/OpticalInterface.cpp
    src/telescope/TelescopeProject.cpp
    src/ui/UI.cpp
    src/raytracer/BundleGenerators.cpp
    src/raytracer/Raytracer.cpp
    src/raytracer/TraceController.cpp
)

set(OPTICFORGE_SHADERS
    src/shaders/grid.vert
    src/shaders/grid.frag
    src/shaders/plane.vert
    src/shaders/plane.frag
)

source_group("Shaders" FILES ${OPTICFORGE_SHADERS})

# Add source to this project's executable.
add_executable (opticforge ${OPTICFORGE_SOURCES} ${OPTICFORGE_SHADERS})

target_compile_features(opticforge PRIVATE cxx_std_17)

target_include_directories(opticforge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)


target_link_libraries(opticforge PRIVATE
    SDL3::SDL3
    libglew_static
    imgui
    implot
    nlohmann_json::nlohmann_json
    glm::glm
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(TBB CONFIG REQUIRED)
    target_link_libraries(opticforge PRIVATE TBB::tbb)
endif()

# -----------------------------------------------------------------------------
#  Compiler settings
# -----------------------------------------------------------------------------

if(MSVC)
    target_compile_options(opticforge PRIVATE
        /W4 /permissive- /utf-8 /MP
        /wd4100        # unreferenced formal parameter -- noisy with callbacks
    )
    target_compile_definitions(opticforge PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(opticforge PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()


# -----------------------------------------------------------------------------
#  Visual Studio conveniences
# -----------------------------------------------------------------------------

if(MSVC)
    # F5 works out of the box instead of launching ALL_BUILD.
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                 PROPERTY VS_STARTUP_PROJECT opticforge)

    # Run from the output dir so config files and shaders resolve.
    set_target_properties(opticforge PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:opticforge>")

    # Mirror the on-disk layout in Solution Explorer.
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src
                 PREFIX "src" FILES ${OPTICFORGE_SOURCES})

    # Keep FetchContent's targets out of the way.
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    foreach(T SDL3-shared SDL_uclibc libglew_static imgui implot)
        if(TARGET ${T})
            set_target_properties(${T} PROPERTIES FOLDER "third_party")
        endif()
    endforeach()
endif()

# Copy SDL3.dll (and anything else shared) next to the exe. Requires 3.21+.
#
# Windows only, and not merely as an optimisation: TARGET_RUNTIME_DLLS is empty
# on platforms without DLLs, so the command collapses to
#     cmake -E copy_if_different <destination>
# with no sources. That is invalid usage, and because it runs POST_BUILD the
# link succeeds first -- producing a build that reports success and then dies
# printing cmake's help text.
#
# Nothing is needed on Linux or macOS: SDL3.so is found through the RPATH CMake
# already sets on the executable.
if(WIN32)
    add_custom_command(TARGET opticforge POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:opticforge> $<TARGET_FILE_DIR:opticforge>
        COMMAND_EXPAND_LISTS)
endif()
add_custom_command(
    TARGET opticforge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders
            $<TARGET_FILE_DIR:opticforge>/shaders
)




if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET opticforge PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
