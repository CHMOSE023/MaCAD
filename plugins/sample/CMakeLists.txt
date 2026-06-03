# Sample dynamically-loaded plugin. Built as a shared library that the host
# discovers at runtime via PluginRegistry::loadPluginLibrary().
add_library(macad_sample_plugin SHARED
    SamplePlugin.cpp)

# A plugin only needs the plugin + core headers (interfaces + logging).
# It links them so the sample is self-contained; a production plugin ABI would
# instead resolve host symbols, but that is out of scope here.
target_link_libraries(macad_sample_plugin PRIVATE
    macad::plugin
    macad::core)

target_include_directories(macad_sample_plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

# Drop the .dll next to MaCAD.exe so the runtime can find it by name.
set_target_properties(macad_sample_plugin PROPERTIES
    FOLDER "plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
