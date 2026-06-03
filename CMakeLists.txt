cmake_minimum_required(VERSION 3.24)

project(MaCAD
    VERSION 0.1.0
    DESCRIPTION "Parametric 3D CAD platform (OCCT + bgfx + ImGui)"
    LANGUAGES CXX)

# ---- Global settings -------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

# Group targets in IDE folders (Visual Studio).
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# MSVC: compile translation units in parallel within each project (/MP). Big
# win under the Visual Studio generator, which otherwise serializes per-project.
if(MSVC)
    add_compile_options(/MP)
endif()

# ---- Options ---------------------------------------------------------------
# How OpenCASCADE is provided:
#   prebuilt -> find_package(OpenCASCADE) from an installed/extracted SDK
#   fetch    -> FetchContent the OCCT sources (heavy: 30-60+ min first build)
set(MACAD_OCCT_SOURCE "prebuilt" CACHE STRING "OCCT source mode: prebuilt | fetch")
set_property(CACHE MACAD_OCCT_SOURCE PROPERTY STRINGS prebuilt fetch)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(cmake/Dependencies.cmake)
include(cmake/OCCT.cmake)
include(cmake/Shaders.cmake)

# Vendored third-party that needs our build (ImGui's bgfx renderer backend).
add_subdirectory(third_party/imgui_bgfx)

add_subdirectory(src)

# Example dynamically-loaded plugin (M6).
add_subdirectory(plugins/sample)
