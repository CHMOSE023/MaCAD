add_library(macad_sketch STATIC
    Sketch.cpp
    Solver.cpp)

target_sources(macad_sketch PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
    FILES
        SketchPlane.hpp
        Sketch.hpp
        Solver.hpp)

target_include_directories(macad_sketch PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# sketch is pure data + math: core (glm) only. It must NOT see OCCT or bgfx.
# the 2D constraint model is independent of both the kernel and the renderer.
target_link_libraries(macad_sketch PUBLIC macad::core)

set_target_properties(macad_sketch PROPERTIES FOLDER "macad")
add_library(macad::sketch ALIAS macad_sketch)
