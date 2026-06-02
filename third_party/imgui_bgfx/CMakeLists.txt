# Vendored Dear ImGui rendering backend for bgfx.
add_library(imgui_bgfx STATIC
    imgui_impl_bgfx.cpp)

target_include_directories(imgui_bgfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(imgui_bgfx PUBLIC imgui bgfx PRIVATE bx)
set_target_properties(imgui_bgfx PROPERTIES FOLDER "third_party")

# Compile ImGui shaders to embeddable headers (dxbc/vs_imgui.sc.bin.h, ...).
macad_add_shaders(imgui_bgfx
    SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    VERTEX     ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vs_imgui.sc
    FRAGMENT   ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fs_imgui.sc)
