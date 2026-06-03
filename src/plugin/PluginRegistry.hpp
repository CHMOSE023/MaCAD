#pragma once

// Central registry of commands and plugins. Supports both static (in-process)
// registration and dynamic loading from shared libraries (M6).

#include "plugin/ICommand.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace macad {

    class IPlugin;

    class PluginRegistry {
    public:
        ~PluginRegistry();

        // Registers a command; later calls with the same id replace the prior one.
        void registerCommand(std::shared_ptr<ICommand> command);

        // Looks up a command by id, or nullptr if absent.
        ICommand* command(const std::string& id) const;

        // All registered commands, for building toolbars/menus.
        std::vector<ICommand*> commands() const;

        // Registers an in-process plugin (calls plugin->registerWith(*this)).
        void registerPlugin(std::shared_ptr<IPlugin> plugin);

        // ---- M6: dynamic loading -------------------------------------------
        // Loads a shared library, resolves the "macadCreatePlugin" factory,
        // instantiates the plugin and registers it. Returns true on success.
        // The library stays loaded until unloadAll() / destruction.
        bool loadPluginLibrary(const std::filesystem::path& path);

        // Names of plugins currently loaded (static + dynamic), for the UI.
        std::vector<std::string> loadedPluginNames() const;

        // Calls unload() on every plugin and frees all dynamic libraries.
        // Must run before the rest of the app tears down (commands may point
        // into plugin code).
        void unloadAll();

    private:
        // An open OS library handle, tagged with the plugin it produced.
        struct LoadedLibrary {
            std::string                 path;
            void*                       handle{ nullptr };  // HMODULE / void*
            std::shared_ptr<IPlugin>    plugin;
        };

        std::map<std::string, std::shared_ptr<ICommand>> m_commands;
        std::vector<std::shared_ptr<IPlugin>>            m_plugins;       // static
        std::vector<LoadedLibrary>                       m_libraries;     // dynamic
    };

} // namespace macad
