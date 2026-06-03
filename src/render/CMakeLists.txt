add_library(macad_render STATIC
    Renderer.cpp
    Camera.cpp
    Mesh.cpp)

target_sources(macad_render PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
    FILES
        Renderer.hpp
        Camera.hpp
        Mesh.hpp)

target_include_directories(macad_render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# render uses bgfx + core only. It must NOT see OCCT.
target_link_libraries(macad_render
    PUBLIC  macad::core
    PRIVATE bgfx::bgfx bgfx::bx bgfx::bimg)

set_target_properties(macad_render PROPERTIES FOLDER "macad")
add_library(macad::render ALIAS macad_render)

# Compile the mesh shaders to embeddable headers (dxbc/vs_mesh.sc.bin.h, ...).
macad_add_shaders(macad_render
    SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    VERTEX     ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vs_mesh.sc
    FRAGMENT   ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fs_mesh.sc)
