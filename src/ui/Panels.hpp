#pragma once

#include "core/AsmTypes.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace macad
{
    class PluginRegistry;
    class CommandStack;
    class ParameterTable;
}

namespace macad::ui
{

    struct FrameStats
    {
        double        fps{ 0.0 };
        std::string   backend;
        std::uint32_t vertexCount{ 0 };
        std::uint32_t triangleCount{ 0 };
    };

    struct FeatureInfo
    {
        std::string   name;
        std::uint32_t triangleCount{ 0 };
    };

    // Bitmask returned by Panels::draw — tells the app what changed.
    enum PanelDirty : unsigned {
        kDirtyNone        = 0,
        kDirtyParams      = 1 << 0,   // parameter value edited → recompute
        kDirtyTransforms  = 1 << 1,   // feature transform edited → updateTransform + solve
        kDirtyConstraints = 1 << 2,   // constraint added/removed/edited → solve
        kDirtyAssembly    = 1 << 3,   // component/mate edited → solveMates
    };

    class Panels
    {
    public:
        // All panels for one frame.
        // selectedFeature: index into features[], -1 = none (in/out).
        // transforms:      one FeatureTransform per feature (in/out).
        // constraints:     assembly constraint list (in/out).
        // Returns OR of PanelDirty flags.
        static unsigned draw(
            PluginRegistry&                  registry,
            CommandStack&                    history,
            const FrameStats&                stats,
            const std::vector<FeatureInfo>&  features,
            ParameterTable&                  params,
            int&                             selectedFeature,
            std::vector<FeatureTransform>&   transforms,
            std::vector<AsmConstraint>&      constraints,
            // M5 full assembly (in/out).
            std::vector<Component>&          components,
            std::vector<Mate>&               mates,
            const AssemblyStatus&            asmStatus,
            int&                             selectedComponent);
    };

} // namespace macad::ui
