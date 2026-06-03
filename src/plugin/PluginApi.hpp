#pragma once

// Cross-platform export/import macro for dynamically loaded plugins, plus the
// C factory ABI the loader resolves.
//
// A plugin shared library must define MACAD_PLUGIN_EXPORTS before including
// this header (the CMake target does so), then export the factory:
//
//   extern "C" MACAD_PLUGIN_API macad::IPlugin* macadCreatePlugin();
//
// The loader (PluginRegistry::loadPluginLibrary) resolves "macadCreatePlugin",
// calls it once, and takes ownership of the returned IPlugin*.

#if defined(_WIN32)
  #if defined(MACAD_PLUGIN_EXPORTS)
    #define MACAD_PLUGIN_API __declspec(dllexport)
  #else
    #define MACAD_PLUGIN_API __declspec(dllimport)
  #endif
#else
  #define MACAD_PLUGIN_API __attribute__((visibility("default")))
#endif

namespace macad {
    class IPlugin;
}

// The exact symbol name the loader looks up.
#define MACAD_PLUGIN_FACTORY_SYMBOL "macadCreatePlugin"

// Function-pointer type matching the exported factory.
extern "C" {
    using MacadCreatePluginFn = macad::IPlugin* (*)();
}
