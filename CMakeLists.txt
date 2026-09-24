# CMakeList.txt : CMake project for opticforge, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.28)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(OPTICFORGE_VERSION "0.1.0")
set(OPTICFORGE_VERSION_FULL "0.1.0-nogit")

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --always
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE  OPTICFORGE_GIT_DESCRIBE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(OPTICFORGE_GIT_DESCRIBE)
        set(OPTICFORGE_VERSION_FULL "${OPTICFORGE_GIT_DESCRIBE}")
        # v1.2.3 / v1.2.3-4-gabcdef / v1.2.3-dirty  ->  1.2.3
        if(OPTICFORGE_GIT_DESCRIBE MATCHES "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)")
            set(OPTICFORGE_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
        else()
            # describe fell back to a bare commit hash: no tag is reachable.
            message(STATUS "No version tag reachable; using ${OPTICFORGE_VERSION}")
        endif()
    endif()
endif()

message(STATUS "OpticForge version: ${OPTICFORGE_VERSION} (${OPTICFORGE_VERSION_FULL})")


project (OpticForge
   VERSION ${OPTICFORGE_VERSION}
   LANGUAGES CXX)
   
# Available to the application, so an About box or a log line can report the
# exact build rather than a number someone remembered to bump.
add_compile_definitions(OPTICFORGE_VERSION_STRING="${OPTICFORGE_VERSION}\(${OPTICFORGE_VERSION_FULL}\)")

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
set(ONETBB_TAG    "v2022.0.0")
set(JSON_SCHEMA_VALIDATOR_TAG "2.4.0")


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
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL)

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

    
# --- JSON Schema Validator ----------------------------------------------------
# JSON Schema validation for OpticForge project files.
#
# This library uses nlohmann/json internally. Since the nlohmann_json target
# already exists in this project, it will reuse our pinned version rather than
# fetching another copy.
set(JSON_VALIDATOR_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(JSON_VALIDATOR_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JSON_VALIDATOR_INSTALL        OFF CACHE BOOL "" FORCE)
set(JSON_VALIDATOR_SHARED_LIBS    OFF CACHE BOOL "" FORCE)

FetchContent_Declare(json_schema_validator
    GIT_REPOSITORY https://github.com/pboettch/json-schema-validator.git
    GIT_TAG        ${JSON_SCHEMA_VALIDATOR_TAG}
    GIT_SHALLOW    TRUE)


# --- GLM ----------------------------------------------------------------------
# Header-only mathematics library used by the renderer/camera code.
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        ${GLM_TAG}
    GIT_SHALLOW    TRUE)

# GCC/libstdc++ before 14 needs direct oneTBB; newer libstdc++ uses it through
# std::execution::par. MSVC uses its own standard-library parallel backend.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(TBB_TEST OFF CACHE BOOL "" FORCE)
    set(TBB_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(TBBMALLOC_BUILD OFF CACHE BOOL "" FORCE)
    # GCC 13 diagnoses an upstream oneTBB atomic operation as a false
    # positive stringop-overflow; do not turn dependency warnings into errors.
    set(TBB_STRICT OFF CACHE BOOL "" FORCE)
    set(TBB_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(oneTBB
        GIT_REPOSITORY https://github.com/oneapi-src/oneTBB.git
        GIT_TAG        ${ONETBB_TAG}
        GIT_SHALLOW    TRUE)
endif()

    
# CMake 4 removed compatibility with projects declaring cmake_minimum_required
# below 3.5, and glew-cmake still declares an older minimum. This is the
# sanctioned escape hatch, scoped to the dependency build so our own project
# keeps its real minimum. Unknown and harmless on CMake < 3.31.
set(_opticforge_saved_policy_min "${CMAKE_POLICY_VERSION_MINIMUM}")
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_MakeAvailable(SDL3 glew imgui implot json json_schema_validator glm)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    FetchContent_MakeAvailable(oneTBB)
endif()

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
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
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
    src/renderer/camera.cpp
    src/renderer/camera.h
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
    src/optics/SurfaceSeparation.cpp
    src/telescope/TelescopeProject.cpp
    src/ui/UI.cpp
    src/ui/TraceSettingsWindow.cpp
    src/raytracer/BundleGenerators.cpp
    src/raytracer/Raytracer.cpp
    src/raytracer/TraceController.cpp
    src/renderer/PsfRasterizer.h
    src/renderer/PsfRasterizer.cpp
    src/renderer/PsfTextureRenderer.h
    src/renderer/PsfTextureRenderer.cpp
    src/renderer/RayPathRenderer.cpp
    src/renderer/RayPathGeometry.cpp
    src/renderer/PickingFrameBuffer.cpp
    src/project/ProjectSerializer.cpp
    src/project/ProjectIO.cpp
    src/project/ProjectController.cpp
    
)

set(OPTICFORGE_SHADERS
    src/shaders/grid.vert
    src/shaders/grid.frag
    src/shaders/plane.vert
    src/shaders/plane.frag
    src/shaders/planeCirc.vert
    src/shaders/planeCirc.frag
    src/shaders/mesh.vert
    src/shaders/mesh.frag
    src/shaders/raypath.vert
    src/shaders/raypath.frag
    src/shaders/picking.vert
    src/shaders/picking.frag
)

source_group("Shaders" FILES ${OPTICFORGE_SHADERS})

# -----------------------------------------------------------------------------
# Embedded OpticForge project schema
# -----------------------------------------------------------------------------

set(OPTICFORGE_PROJECT_SCHEMA_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/project-schema.json"
)

set(OPTICFORGE_GENERATED_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

file(MAKE_DIRECTORY "${OPTICFORGE_GENERATED_DIR}")

# Read the authoritative JSON schema.
file(READ
    "${OPTICFORGE_PROJECT_SCHEMA_FILE}"
    OPTICFORGE_PROJECT_SCHEMA
)

# project-schema.json is consumed through file(READ), so explicitly tell CMake
# that changing it requires reconfiguration.
set_property(
    DIRECTORY
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${OPTICFORGE_PROJECT_SCHEMA_FILE}"
)

# Generate a C++ source file containing the schema.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbeddedProjectSchema.cpp.in"
    "${OPTICFORGE_GENERATED_DIR}/EmbeddedProjectSchema.cpp"
    @ONLY
)
set(OPTICFORGE_GENERATED_SOURCES ${OPTICFORGE_GENERATED_DIR}/EmbeddedProjectSchema.cpp)
# Add source to this project's executable.
add_executable (opticforge ${OPTICFORGE_SOURCES} ${OPTICFORGE_SHADERS} ${OPTICFORGE_GENERATED_SOURCES})

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
    nlohmann_json_schema_validator::validator
    glm::glm
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
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

# Focused optical regression tests. No GUI or OpenGL context is required.
include(CTest)
if(BUILD_TESTING)
    add_executable(opticforge_raytracer_tests
        tests/RaytracerTests.cpp
        src/raytracer/Raytracer.cpp
        src/optics/ConicGeometry.cpp
        src/optics/PlaneGeometry.cpp
        src/optics/Transform.cpp
        src/optics/Aperture.cpp
        src/optics/OpticalSurface.cpp
        src/optics/OpticalInterface.cpp)
    target_include_directories(opticforge_raytracer_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(opticforge_raytracer_tests PRIVATE glm::glm)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(opticforge_raytracer_tests PRIVATE TBB::tbb)
    endif()
    add_test(NAME raytracer_regressions COMMAND opticforge_raytracer_tests)
endif()
# -----------------------------------------------------------------------------
#  Install and package
# -----------------------------------------------------------------------------
 
install(TARGETS opticforge RUNTIME DESTINATION .)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/shaders/"
DESTINATION shaders
)
 
# SDL3.dll and anything else CMake knows we link. Vendor SDKs are invisible here
# by design -- they are loaded at runtime, so CMake has no dependency edge to
# them and they must be handled separately above.
if(WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:opticforge> DESTINATION . OPTIONAL)
else()
    # TARGET_RUNTIME_DLLS is Windows-only, but SDL3 is a shared library here too.
    # Install the target itself so CMake resolves the versioned soname and the
    # symlink chain -- copying $<TARGET_FILE:SDL3::SDL3> alone gives you
    # libSDL3.so.0.2.0 with nothing pointing at it.
    install(TARGETS SDL3-shared
                LIBRARY DESTINATION lib
                        COMPONENT opticforge
                        NAMELINK_SKIP)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # The installed executable uses $ORIGIN/lib; ship the TBB soname too.
        install(TARGETS tbb LIBRARY DESTINATION lib COMPONENT opticforge)
    endif()
endif()

if(NOT WIN32 AND NOT APPLE)
    set_target_properties(opticforge PROPERTIES
        # $ORIGIN is resolved at load time relative to the executable, so the
        # tree relocates anywhere. The build-tree RPATH CMake adds by default is
        # an absolute path into the build directory and is useless once shipped.
        INSTALL_RPATH "$ORIGIN/lib"
        BUILD_WITH_INSTALL_RPATH FALSE)
endif()
 
install(FILES
    "${CMAKE_SOURCE_DIR}/README.md"
    "${CMAKE_SOURCE_DIR}/LICENSE"
    DESTINATION . OPTIONAL)
if(EXISTS "${CMAKE_SOURCE_DIR}/THIRD_PARTY.md")
    install(FILES "${CMAKE_SOURCE_DIR}/THIRD_PARTY.md" DESTINATION .)
endif()
 
if(MSVC)
    # The VC++ runtime, so the installer does not depend on the user already
    # having the redistributable.
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
    include(InstallRequiredSystemLibraries)
endif()
 
set(CPACK_PACKAGE_NAME              "OpticForge")
set(CPACK_PACKAGE_VENDOR            "OpticForge Project")
set(CPACK_PACKAGE_VERSION           "${OPTICFORGE_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open-source optics design software")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "OpticForge")
set(CPACK_PACKAGE_FILE_NAME
    "OpticForge-${OPTICFORGE_VERSION_FULL}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
endif()
 
if(WIN32)
    # ZIP first: it always works, needs no external tooling, and is what CI can
    # produce unconditionally. NSIS is added only when makensis is present.
    set(CPACK_GENERATOR "ZIP")
    find_program(MAKENSIS_EXECUTABLE NAMES makensis)
    if(MAKENSIS_EXECUTABLE)
        list(APPEND CPACK_GENERATOR "NSIS")
        set(CPACK_NSIS_PACKAGE_NAME       "OpticForge ${OPTICFORGE_VERSION}")
        set(CPACK_NSIS_DISPLAY_NAME       "OpticForge optics design software")
        set(CPACK_NSIS_INSTALLED_ICON_NAME "opticforge.exe")
        set(CPACK_NSIS_MODIFY_PATH        OFF)
        set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
        set(CPACK_NSIS_CREATE_ICONS_EXTRA
            "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\OpticForge.lnk' '$INSTDIR\\\\opticforge.exe'")
        set(CPACK_NSIS_DELETE_ICONS_EXTRA
            "Delete '$SMPROGRAMS\\\\$START_MENU\\\\OpticForge.lnk'")
    else()
        message(STATUS "makensis not found -- ZIP package only")
    endif()
else()
    set(CPACK_GENERATOR "TGZ")
endif()
 
include(CPack)
