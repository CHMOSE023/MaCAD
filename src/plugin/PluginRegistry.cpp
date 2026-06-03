#include "plugin/PluginRegistry.hpp"

#include "plugin/IPlugin.hpp"
#include "plugin/PluginApi.hpp"
#include "core/Log.hpp"

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <dlfcn.h>
#endif

namespace macad
{
    namespace
    {
        // ---- Thin OS-library wrappers --------------------------------------
        void* osLoad(const std::filesystem::path& p)
        {
#if defined(_WIN32)
            return reinterpret_cast<void*>(::LoadLibraryW(p.wstring().c_str()));
#else
            return ::dlopen(p.string().c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
        }

        void* osSymbol(void* handle, const char* name)
        {
#if defined(_WIN32)
            return reinterpret_cast<void*>(
                ::GetProcAddress(reinterpret_cast<HMODULE>(handle), name));
#else
            return ::dlsym(handle, name);
#endif
        }

        void osFree(void* handle)
        {
#if defined(_WIN32)
            ::FreeLibrary(reinterpret_cast<HMODULE>(handle));
#else
            ::dlclose(handle);
#endif
        }

        std::string osError()
        {
#if defined(_WIN32)
            const DWORD code = ::GetLastError();
            return "WinAPI error " + std::to_string(code);
#else
            const char* e = ::dlerror();
            return e ? std::string(e) : "unknown dlerror";
#endif
        }
    } // namespace

    PluginRegistry::~PluginRegistry()
    {
        unloadAll();
    }

    void PluginRegistry::registerCommand(std::shared_ptr<ICommand> command)
    {
        if (!command) return;
        const std::string id = command->id();
        m_commands[id]       = std::move(command);
        MACAD_LOG_DEBUG("Registered command '{}'", id);
    }

    ICommand* PluginRegistry::command(const std::string& id) const
    {
        const auto it = m_commands.find(id);
        return it != m_commands.end() ? it->second.get() : nullptr;
    }

    std::vector<ICommand*> PluginRegistry::commands() const
    {
        std::vector<ICommand*> out;
        out.reserve(m_commands.size());
        for (const auto& [id, cmd] : m_commands)
            out.push_back(cmd.get());
        return out;
    }

    void PluginRegistry::registerPlugin(std::shared_ptr<IPlugin> plugin)
    {
        if (!plugin) return;
        MACAD_LOG_INFO("Registering plugin '{}' v{}", plugin->name(), plugin->version());
        plugin->registerWith(*this);
        m_plugins.push_back(std::move(plugin));
    }

    bool PluginRegistry::loadPluginLibrary(const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path))
        {
            MACAD_LOG_ERROR("loadPluginLibrary: file not found: {}", path.string());
            return false;
        }

        void* handle = osLoad(path);
        if (!handle)
        {
            MACAD_LOG_ERROR("loadPluginLibrary: load failed for {}: {}",
                            path.string(), osError());
            return false;
        }

        void* sym = osSymbol(handle, MACAD_PLUGIN_FACTORY_SYMBOL);
        if (!sym)
        {
            MACAD_LOG_ERROR("loadPluginLibrary: missing symbol '{}' in {}",
                            MACAD_PLUGIN_FACTORY_SYMBOL, path.string());
            osFree(handle);
            return false;
        }

        auto factory = reinterpret_cast<MacadCreatePluginFn>(sym);
        IPlugin* raw = factory();
        if (!raw)
        {
            MACAD_LOG_ERROR("loadPluginLibrary: factory returned null in {}",
                            path.string());
            osFree(handle);
            return false;
        }

        // Wrap in shared_ptr; the library handle outlives the plugin, so we
        // delete the plugin first then free the library in unloadAll().
        std::shared_ptr<IPlugin> plugin(raw);
        MACAD_LOG_INFO("Loaded plugin '{}' v{} from {}",
                       plugin->name(), plugin->version(), path.filename().string());
        plugin->registerWith(*this);

        m_libraries.push_back(LoadedLibrary{
            path.string(), handle, std::move(plugin) });
        return true;
    }

    std::vector<std::string> PluginRegistry::loadedPluginNames() const
    {
        std::vector<std::string> names;
        for (const auto& p : m_plugins)
            names.push_back(p->name() + " (static)");
        for (const auto& lib : m_libraries)
            names.push_back(lib.plugin->name() + " (dll)");
        return names;
    }

    void PluginRegistry::unloadAll()
    {
        // Commands contributed by dynamic plugins were allocated inside those
        // DLLs (their shared_ptr deleter points into DLL code). They MUST be
        // destroyed before any library is unmapped, so clear all commands
        // first. The host re-registers its built-ins on next startup; at
        // teardown this is simply the safe order.
        if (!m_libraries.empty())
            m_commands.clear();

        // Static plugins: just notify; they are owned by this registry.
        for (auto& p : m_plugins)
            if (p) p->unload();
        m_plugins.clear();

        // Dynamic: notify, drop the plugin instance, THEN free the library.
        for (auto& lib : m_libraries)
        {
            if (lib.plugin)
            {
                lib.plugin->unload();
                lib.plugin.reset();   // destroy plugin while its code is still mapped
            }
            if (lib.handle)
            {
                osFree(lib.handle);
                lib.handle = nullptr;
            }
        }
        m_libraries.clear();
    }

} // namespace macad
