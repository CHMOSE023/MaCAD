add_library(macad_geometry STATIC
    Shape.cpp
    Primitives.cpp
    Tessellator.cpp)

target_sources(macad_geometry PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
    FILES
        Shape.hpp
        Primitives.hpp
        Tessellator.hpp)

target_include_directories(macad_geometry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# geometry is the ONLY layer allowed to link OCCT.
target_link_libraries(macad_geometry
    PUBLIC  macad::core
    PRIVATE macad::occt)

set_target_properties(macad_geometry PROPERTIES FOLDER "macad")
add_library(macad::geometry ALIAS macad_geometry)
