add_library(macad_ui STATIC
    ImGuiLayer.cpp
    Panels.cpp)

target_sources(macad_ui PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
    FILES
        ImGuiLayer.hpp
        Panels.hpp)

target_include_directories(macad_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# ui uses ImGui (+ our bgfx backend), GLFW, plugin (for command toolbar) and
# core. It must NOT see OCCT.
target_link_libraries(macad_ui
    PUBLIC  macad::core macad::plugin
    PRIVATE imgui imgui_bgfx glfw)

set_target_properties(macad_ui PROPERTIES FOLDER "macad")
add_library(macad::ui ALIAS macad_ui)
