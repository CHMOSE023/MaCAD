// Sample dynamically-loaded MaCAD plugin.
//
// Built as a shared library (macad_sample_plugin.dll). At runtime the host
// resolves "macadCreatePlugin", instantiates this plugin, and calls
// registerWith() so the plugin can contribute commands to the toolbar.
//
// This sample contributes a single command that just logs a message — enough
// to prove the full dynamic-loading round trip without depending on the
// geometry or render layers (a plugin only needs the plugin + core headers).

#define MACAD_PLUGIN_EXPORTS
#include "plugin/PluginApi.hpp"
#include "plugin/IPlugin.hpp"
#include "plugin/ICommand.hpp"
#include "plugin/PluginRegistry.hpp"
#include "core/Log.hpp"

#include <memory>
#include <string>

namespace {

    // A trivial command the plugin contributes.
    class HelloCommand : public macad::ICommand {
    public:
        std::string id()    const override { return "sample.hello"; }
        std::string label() const override { return "Sample: Hello"; }
        void execute() override {
            MACAD_LOG_INFO("[SamplePlugin] Hello from a dynamically loaded plugin!");
        }
    };

    class SamplePlugin : public macad::IPlugin {
    public:
        std::string name()    const override { return "Sample Plugin"; }
        std::string version() const override { return "1.0.0"; }

        void registerWith(macad::PluginRegistry& registry) override {
            registry.registerCommand(std::make_shared<HelloCommand>());
            MACAD_LOG_INFO("[SamplePlugin] registered command 'sample.hello'");
        }

        void unload() override {
            MACAD_LOG_INFO("[SamplePlugin] unloading");
        }
    };

} // namespace

// The factory symbol the loader resolves. Host takes ownership of the result.
extern "C" MACAD_PLUGIN_API macad::IPlugin* macadCreatePlugin() {
    return new SamplePlugin();
}
