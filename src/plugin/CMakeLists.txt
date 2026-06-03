# Plugin interface layer + static registry + dynamic library loader (M6).
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
        CommandStack.hpp
        PluginApi.hpp)

target_include_directories(macad_plugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(macad_plugin PUBLIC macad::core)
# dlopen/dlsym live in libdl on Linux.
if(UNIX AND NOT APPLE)
    target_link_libraries(macad_plugin PUBLIC dl)
endif()
set_target_properties(macad_plugin PROPERTIES FOLDER "macad")
add_library(macad::plugin ALIAS macad_plugin)
