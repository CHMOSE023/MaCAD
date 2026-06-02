# Central declaration of all FetchContent-managed third-party dependencies.
# Heavy OCCT is handled separately in cmake/OCCT.cmake.

include(FetchContent)

# Be quiet and reuse already-populated content when possible.
set(FETCHCONTENT_QUIET OFF)

# ---- glm (header-only math) -----------------------------------------------
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE)

# ---- spdlog (logging) ------------------------------------------------------
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    GIT_SHALLOW    TRUE)

# ---- GLFW (windowing/input) ------------------------------------------------
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE)

# ---- bgfx.cmake (bgfx + bx + bimg + shaderc) -------------------------------
# This wrapper repo provides CMake targets for the bgfx family and the
# `shaderc` tool used to compile our .sc shaders at build time.
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL        OFF CACHE BOOL "" FORCE)

# Build only the bgfx tools we actually use. We need shaderc (compiles our .sc
# shaders) and keep bin2c; the geometry/texture tools and their viewers
# (geometryc/geometryv/texturec/texturev + example-common) are dead weight that
# dominate first-build time, so disable them.
set(BGFX_BUILD_TOOLS          ON  CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_SHADER   ON  CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_BIN2C    ON  CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_GEOMETRY OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_TEXTURE  OFF CACHE BOOL "" FORCE)
# Amalgamated bgfx core compiles noticeably faster.
set(BGFX_AMALGAMATED          ON  CACHE BOOL "" FORCE)

# bgfx pulls bx/bimg/bgfx as git submodules (large repos). On flaky networks the
# ExternalProject clone (only 2 retries, full depth) fails. If a pre-populated
# local copy exists at .deps/bgfx.cmake (clone it shallow with submodules), use
# it directly and skip network entirely.
if(EXISTS "${CMAKE_SOURCE_DIR}/.deps/bgfx.cmake/CMakeLists.txt")
    set(FETCHCONTENT_SOURCE_DIR_BGFX "${CMAKE_SOURCE_DIR}/.deps/bgfx.cmake"
        CACHE PATH "Local bgfx.cmake checkout" FORCE)
    message(STATUS "Using local bgfx.cmake at ${FETCHCONTENT_SOURCE_DIR_BGFX}")
endif()

FetchContent_Declare(bgfx
    GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
    GIT_TAG        v1.143.9262-545
    GIT_SHALLOW    TRUE)

# ---- Dear ImGui (no upstream CMake: we build it ourselves) -----------------
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.5
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(glm spdlog glfw bgfx imgui)

# ---- Build an ImGui static library target ----------------------------------
# ImGui ships no CMakeLists, so we assemble a target. We use the GLFW platform
# backend; the *rendering* backend for bgfx is vendored under third_party
# (ImGui has no official bgfx backend).
if(NOT TARGET imgui)
    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp)
    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends)
    target_link_libraries(imgui PUBLIC glfw)
    set_target_properties(imgui PROPERTIES FOLDER "third_party")
endif()

# Expose the imgui source dir to the rest of the build (used by the vendored
# bgfx rendering backend under third_party/imgui_bgfx).
set(MACAD_IMGUI_SOURCE_DIR "${imgui_SOURCE_DIR}" CACHE INTERNAL "ImGui source dir")

# When building bgfx from source (FetchContent), BGFX_SHADER_INCLUDE_PATH (set
# only by the installed package config) is empty, so shaderc cannot find
# bgfx_shader.sh. Expose the in-tree shader include dir for our Shaders.cmake.
if(DEFINED bgfx_SOURCE_DIR)
    set(MACAD_BGFX_SHADER_INCLUDE "${bgfx_SOURCE_DIR}/bgfx/src"
        CACHE INTERNAL "bgfx shader include dir (bgfx_shader.sh)")
endif()
