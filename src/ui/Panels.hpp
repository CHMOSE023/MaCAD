#pragma once

// Immediate-mode panels for Milestone 1: a command toolbar (driven by the
// plugin registry), a placeholder feature tree, and a stats overlay. These are
// intentionally thin; richer docking layouts arrive with later milestones.

#include <cstdint>
#include <string>
#include <vector>

namespace macad
{
    class PluginRegistry;
    class CommandStack;
}

namespace macad::ui
{

    struct FrameStats
    {
        double fps{ 0.0 };
        std::string backend;
        std::uint32_t vertexCount{ 0 };
        std::uint32_t triangleCount{ 0 };
    };

    // One entry in the feature tree (passed from app layer, no GPU types here).
    struct FeatureInfo 
    {
        std::string name;
        std::uint32_t triangleCount{ 0 };
    };

    class Panels
    {
    public:
        // Draws all panels for the frame. Commands clicked in the toolbar are
        // routed through the CommandStack so they are undoable when applicable.
        // `features` is the current list of built solids for the feature tree.
        static void draw(PluginRegistry&   registry,
                         CommandStack&     history,
                         const FrameStats& stats,
                         const std::vector<FeatureInfo>& features);
    };

} // namespace macad::ui
