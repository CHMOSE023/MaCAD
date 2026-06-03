add_executable(MaCAD   main.cpp  Application.cpp)

target_include_directories(MaCAD PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

# The app is the assembly layer: it links every module.
target_link_libraries(MaCAD PRIVATE
    macad::core
    macad::geometry
    macad::render
    macad::ui
    macad::plugin
    glfw
    imgui::imgui)  # app drives camera input via ImGui IO (WantCaptureMouse, MouseDelta)

set_target_properties(MaCAD PROPERTIES
    FOLDER "macad"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

if(WIN32)
    target_compile_definitions(MaCAD PRIVATE GLFW_EXPOSE_NATIVE_WIN32 NOMINMAX)
endif()

 
