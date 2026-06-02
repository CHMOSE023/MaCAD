#pragma once

// Thin wrapper over spdlog. Provides a single application logger plus
// MACAD_LOG_* convenience macros. Call macad::Log::init() once at startup.

#include <memory>

#include <spdlog/spdlog.h>

namespace macad {

    class Log {
    public:
        // Initializes the global logger (console sink, optional file sink).
        // Safe to call once; subsequent calls are no-ops.
        static void init(bool toFile = false);

        static spdlog::logger& get();

    private:
        static std::shared_ptr<spdlog::logger> s_logger;
    };

} // namespace macad

// Convenience macros. Usage: MACAD_LOG_INFO("box: {} faces", n);
#define MACAD_LOG_TRACE(...) ::macad::Log::get().trace(__VA_ARGS__)
#define MACAD_LOG_DEBUG(...) ::macad::Log::get().debug(__VA_ARGS__)
#define MACAD_LOG_INFO(...)  ::macad::Log::get().info(__VA_ARGS__)
#define MACAD_LOG_WARN(...)  ::macad::Log::get().warn(__VA_ARGS__)
#define MACAD_LOG_ERROR(...) ::macad::Log::get().error(__VA_ARGS__)
