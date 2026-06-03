#include "plugin/PluginRegistry.hpp"

#include "plugin/IPlugin.hpp"
#include "core/Log.hpp"

namespace macad
{

    void PluginRegistry::registerCommand(std::shared_ptr<ICommand> command)
    {
        if (!command)
        {
            return;
        }

        const std::string id = command->id();
        m_commands[id]       = std::move(command);
        MACAD_LOG_DEBUG("Registered command '{}'", id);
    }

    ICommand* PluginRegistry::command(const std::string& id) const 
    {
        const auto it = m_commands.find(id);
        return it     != m_commands.end() ? it->second.get() : nullptr;
    }

    std::vector<ICommand*> PluginRegistry::commands() const
    {
        std::vector<ICommand*> out;
        out.reserve(m_commands.size());
        for (const auto& [id, cmd] : m_commands) 
        {
            out.push_back(cmd.get());
        }
        return out;
    }

    void PluginRegistry::registerPlugin(std::shared_ptr<IPlugin> plugin) 
    {
        if (!plugin) 
        {
            return;
        }
        MACAD_LOG_INFO("Registering plugin '{}' v{}", plugin->name(), plugin->version());
        plugin->registerWith(*this);
        m_plugins.push_back(std::move(plugin));
    }

} 
