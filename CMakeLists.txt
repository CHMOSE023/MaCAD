cmake_minimum_required(VERSION 3.24)

project( MaCAD  VERSION 0.1.0  DESCRIPTION "Parametric 3D CAD " LANGUAGES CXX)

# ------------------------------------------------------------------------------
# Global
# ------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/MP)
endif()

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ------------------------------------------------------------------------------
# Dependencies (vcpkg)
# ------------------------------------------------------------------------------

find_package(glm    CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glfw3  CONFIG REQUIRED)
find_package(imgui  CONFIG REQUIRED)
find_package(bgfx   CONFIG REQUIRED)

if(TARGET bgfx::shaderc)
    get_target_property(MACAD_BGFX_SHADERC bgfx::shaderc IMPORTED_LOCATION)
    if(NOT MACAD_BGFX_SHADERC)
        get_target_property(MACAD_BGFX_SHADERC bgfx::shaderc IMPORTED_LOCATION_RELEASE)
    endif()
endif()

if(NOT MACAD_BGFX_SHADERC OR MACAD_BGFX_SHADERC MATCHES "-NOTFOUND$")
    message(FATAL_ERROR
        "bgfx shader compiler was not found. Install the vcpkg tools feature: "
        "vcpkg install \"bgfx[tools]:${VCPKG_TARGET_TRIPLET}\" --recurse")
endif()

foreach(MACAD_BGFX_TOOL bin2c texturec geometryc shaderc)
    if(TARGET bgfx::${MACAD_BGFX_TOOL})
        set_target_properties(bgfx::${MACAD_BGFX_TOOL} PROPERTIES
            MAP_IMPORTED_CONFIG_DEBUG Release
            MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
            MAP_IMPORTED_CONFIG_MINSIZEREL Release)
    endif()
endforeach()

# ------------------------------------------------------------------------------
# OpenCASCADE (local install)
# ------------------------------------------------------------------------------

find_package(OpenCASCADE REQUIRED)

add_library(macad_occt INTERFACE)
add_library(macad::occt ALIAS macad_occt)

target_include_directories(macad_occt  INTERFACE  ${OpenCASCADE_INCLUDE_DIR} )
target_link_directories   (macad_occt  INTERFACE  ${OpenCASCADE_LIBRARY_DIR} )

target_link_libraries(macad_occt
    INTERFACE
    TKernel
    TKMath
    TKG2d
    TKG3d
    TKGeomBase
    TKBRep
    TKGeomAlgo
    TKTopAlgo
    TKPrim
    TKMesh
)

# ------------------------------------------------------------------------------
# bgfx shader support
# ------------------------------------------------------------------------------

set(MACAD_SHADER_PROFILE  "s_5_0"  CACHE STRING "shader profile")

if(NOT COMMAND bgfx_compile_shaders)

    find_file(
        BGFX_TOOL_UTILS
        bgfxToolUtils.cmake
        PATHS
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/bgfx"
        NO_DEFAULT_PATH
    )

    if(NOT BGFX_TOOL_UTILS)
        message(FATAL_ERROR
            "bgfxToolUtils.cmake not found")
    endif()

    include("${BGFX_TOOL_UTILS}")
endif()

function(macad_add_shaders TARGET)

    set(oneValue SHADER_DIR OUTPUT_DIR)
    set(multiValue VERTEX FRAGMENT)

    cmake_parse_arguments(
        ARG
        ""
        "${oneValue}"
        "${multiValue}"
        ${ARGN}
    )

    if(NOT ARG_OUTPUT_DIR)
        set(ARG_OUTPUT_DIR
            "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    endif()

    target_include_directories(
        ${TARGET}
        PRIVATE
        "${ARG_OUTPUT_DIR}"
    )

    bgfx_compile_shaders(
        TYPE VERTEX
        SHADERS ${ARG_VERTEX}
        VARYING_DEF "${ARG_SHADER_DIR}/varying.def.sc"
        OUTPUT_DIR "${ARG_OUTPUT_DIR}"
        PROFILES ${MACAD_SHADER_PROFILE}
        AS_HEADERS
        OUT_FILES_VAR _vs_outputs
    )

    bgfx_compile_shaders(
        TYPE FRAGMENT
        SHADERS ${ARG_FRAGMENT}
        VARYING_DEF "${ARG_SHADER_DIR}/varying.def.sc"
        OUTPUT_DIR "${ARG_OUTPUT_DIR}"
        PROFILES ${MACAD_SHADER_PROFILE}
        AS_HEADERS
        OUT_FILES_VAR _fs_outputs
    )

    target_sources(
        ${TARGET}
        PRIVATE
        ${_vs_outputs}
        ${_fs_outputs}
    )

endfunction()

# ------------------------------------------------------------------------------
# Third-party
# ------------------------------------------------------------------------------ 
add_subdirectory(third_party/imgui_bgfx)

# ------------------------------------------------------------------------------
# Source
# ------------------------------------------------------------------------------ 
add_subdirectory(src)

# ------------------------------------------------------------------------------
# Plugins
# ------------------------------------------------------------------------------ 
add_subdirectory(plugins/sample)
