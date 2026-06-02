#pragma once

// Plugin entry-point interface. RESERVED: M6 will add dynamic library loading
// (LoadLibrary/dlopen) that discovers a factory symbol returning an IPlugin.
// For now plugins can only be registered statically (in-process).

#include <string>

namespace macad {

    class PluginRegistry;

    class IPlugin {
    public:
        virtual ~IPlugin() = default;

        virtual std::string name() const = 0;
        virtual std::string version() const = 0;

        // Called once after load: the plugin registers its commands/features here.
        virtual void registerWith(PluginRegistry& registry) = 0;

        // Called once before unload for cleanup.
        virtual void unload() {}
    };

    // ---- Dynamic-loading ABI (RESERVED, not implemented in M1) -----------------
    // A dynamically loaded plugin will export a C factory with this signature:
    //
    //   extern "C" MACAD_PLUGIN_API macad::IPlugin* macadCreatePlugin();
    //
    // The loader (M6) will resolve "macadCreatePlugin", call it, and own the
    // returned instance. Kept here so the contract is visible from the start.

} // namespace macad
