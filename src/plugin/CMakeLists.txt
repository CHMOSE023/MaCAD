# Header-mostly plugin interface layer. Dynamic loading is reserved for M6;
# Milestone 1 only ships the interfaces and a static registry.
add_library(macad_plugin STATIC
    PluginRegistry.cpp
    CommandStack.cpp)

target_sources(macad_plugin PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
    FILES
        IPlugin.hpp
        ICommand.hpp
        PluginRegistry.hpp
        CommandStack.hpp)

target_include_directories(macad_plugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(macad_plugin PUBLIC macad::core)
set_target_properties(macad_plugin PROPERTIES FOLDER "macad")
add_library(macad::plugin ALIAS macad_plugin)
