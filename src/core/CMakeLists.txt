add_library(macad_core STATIC
    Log.cpp)

target_sources(macad_core PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
    FILES
        Log.hpp
        Types.hpp
        MeshData.hpp
        Parameter.hpp
        Document.hpp)

target_include_directories(macad_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(macad_core PUBLIC glm::glm spdlog::spdlog)
set_target_properties(macad_core PROPERTIES FOLDER "macad")
add_library(macad::core ALIAS macad_core)
