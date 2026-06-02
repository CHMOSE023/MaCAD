#pragma once

// Immediate-mode panels for Milestone 1: a command toolbar (driven by the
// plugin registry), a placeholder feature tree, and a stats overlay. These are
// intentionally thin; richer docking layouts arrive with later milestones.

#include <cstdint>
#include <string>

namespace macad {
    class PluginRegistry;
}

namespace macad::ui {

    struct FrameStats {
        double fps{ 0.0 };
        std::string backend;
        std::uint32_t vertexCount{ 0 };
        std::uint32_t triangleCount{ 0 };
    };

    class Panels {
    public:
        // Draws all panels for the frame. Commands clicked in the toolbar are
        // executed immediately via the registry.
        static void draw(PluginRegistry& registry, const FrameStats& stats);
    };

} // namespace macad::ui
