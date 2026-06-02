#include "core/Log.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <vector>

namespace macad {

    std::shared_ptr<spdlog::logger> Log::s_logger;

    void Log::init(bool toFile) {
        if (s_logger) {
            return;
        }

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        if (toFile) {
            sinks.push_back(
                std::make_shared<spdlog::sinks::basic_file_sink_mt>("macad.log", true));
        }

        s_logger = std::make_shared<spdlog::logger>("MaCAD", sinks.begin(), sinks.end());
        s_logger->set_level(spdlog::level::trace);
        s_logger->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
        spdlog::register_logger(s_logger);
    }

    spdlog::logger& Log::get() {
        if (!s_logger) {
            init();
        }
        return *s_logger;
    }

} // namespace macad
