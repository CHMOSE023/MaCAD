#pragma once

// Central registry of commands (and, later, feature types). Milestone 1 wires
// only static, in-process registration. Dynamic library discovery is M6 and is
// marked with TODOs against the reserved entry points below.

#include "plugin/ICommand.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace macad {

    class IPlugin;

    class PluginRegistry {
    public:
        // Registers a command; later calls with the same id replace the prior one.
        void registerCommand(std::shared_ptr<ICommand> command);

        // Looks up a command by id, or nullptr if absent.
        ICommand* command(const std::string& id) const;

        // All registered commands, for building toolbars/menus.
        std::vector<ICommand*> commands() const;

        // Registers an in-process plugin (calls plugin->registerWith(*this)).
        void registerPlugin(std::shared_ptr<IPlugin> plugin);

        // TODO(M6): loadPluginLibrary(const std::filesystem::path&) -> resolves the
        // "macadCreatePlugin" factory from a DLL/so and registers the result.

    private:
        std::map<std::string, std::shared_ptr<ICommand>> m_commands;
        std::vector<std::shared_ptr<IPlugin>> m_plugins;
    };

} // namespace macad
