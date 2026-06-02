# Build-time shader compilation for bgfx.
#
# This bgfx.cmake version exposes bgfx_compile_shaders(... AS_HEADERS ...),
# which compiles each .sc to per-profile C headers (bin2c). For Milestone 1 we
# target a single profile (DX11 / s_5_0) and force the Direct3D11 backend in the
# renderer, keeping shader handling simple and the binary self-contained.
#
# For a shader file vs_mesh.sc with profile s_5_0, the helper emits:
#   <OUTPUT_DIR>/dxbc/vs_mesh.sc.bin.h   with array  vs_mesh_dxbc
# so callers include "dxbc/<name>.sc.bin.h" and use the <name>_dxbc array.

# Single profile shared by all shaders here (DX11 bytecode).
set(MACAD_SHADER_PROFILE "s_5_0" CACHE STRING "shaderc profile for embedded shaders")

function(macad_add_shaders TARGET)
    set(oneValue SHADER_DIR OUTPUT_DIR)
    set(multiValue VERTEX FRAGMENT)
    cmake_parse_arguments(ARG "" "${oneValue}" "${multiValue}" ${ARGN})

    if(NOT ARG_OUTPUT_DIR)
        set(ARG_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    endif()
    target_include_directories(${TARGET} PRIVATE "${ARG_OUTPUT_DIR}")

    # The shader helpers live in bgfx.cmake's tool utils; make sure they exist.
    if(NOT COMMAND bgfx_compile_shaders)
        if(DEFINED bgfx_SOURCE_DIR AND EXISTS "${bgfx_SOURCE_DIR}/cmake/bgfxToolUtils.cmake")
            include("${bgfx_SOURCE_DIR}/cmake/bgfxToolUtils.cmake")
        else()
            message(FATAL_ERROR "bgfx_compile_shaders not available (bgfx.cmake too old or not populated)")
        endif()
    endif()

    bgfx_compile_shaders(
        TYPE VERTEX
        SHADERS ${ARG_VERTEX}
        VARYING_DEF "${ARG_SHADER_DIR}/varying.def.sc"
        OUTPUT_DIR "${ARG_OUTPUT_DIR}"
        INCLUDE_DIRS "${MACAD_BGFX_SHADER_INCLUDE}"
        PROFILES ${MACAD_SHADER_PROFILE}
        AS_HEADERS
        OUT_FILES_VAR _vs_outputs)

    bgfx_compile_shaders(
        TYPE FRAGMENT
        SHADERS ${ARG_FRAGMENT}
        VARYING_DEF "${ARG_SHADER_DIR}/varying.def.sc"
        OUTPUT_DIR "${ARG_OUTPUT_DIR}"
        INCLUDE_DIRS "${MACAD_BGFX_SHADER_INCLUDE}"
        PROFILES ${MACAD_SHADER_PROFILE}
        AS_HEADERS
        OUT_FILES_VAR _fs_outputs)

    target_sources(${TARGET} PRIVATE ${_vs_outputs} ${_fs_outputs})
endfunction()
