#include "ui/Panels.hpp"

#include "plugin/PluginRegistry.hpp"
#include "plugin/CommandStack.hpp"
#include "plugin/ICommand.hpp"
#include "core/ParameterTable.hpp"

#include <imgui.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

namespace macad::ui
{

    // -------------------------------------------------------------------------
    // Small helpers
    // -------------------------------------------------------------------------

    // InputText that owns a std::string buffer. Returns true on change.
    static bool inputString(const char* label, std::string& s, float width = 75.0f)
    {
        char buf[128];
        std::strncpy(buf, s.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        ImGui::SetNextItemWidth(width);
        if (ImGui::InputText(label, buf, sizeof(buf)))
        {
            s = buf;
            return true;
        }
        return false;
    }

    // -------------------------------------------------------------------------

    unsigned Panels::draw(
        PluginRegistry&                 registry,
        CommandStack&                   history,
        const FrameStats&               stats,
        const std::vector<FeatureInfo>& features,
        ParameterTable&                 params,
        int&                            selectedFeature,
        std::vector<FeatureTransform>&  transforms,
        std::vector<AsmConstraint>&     constraints)
    {
        unsigned dirty = kDirtyNone;

        // ---- Toolbar -------------------------------------------------------
        if (ImGui::Begin("Toolbar"))
        {
            const bool canUndo = history.canUndo();
            const bool canRedo = history.canRedo();

            ImGui::BeginDisabled(!canUndo);
            if (ImGui::Button("Undo")) history.undo();
            if (canUndo && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("Undo: %s", history.undoLabel().c_str());
            ImGui::EndDisabled();

            ImGui::SameLine();

            ImGui::BeginDisabled(!canRedo);
            if (ImGui::Button("Redo")) history.redo();
            if (canRedo && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("Redo: %s", history.redoLabel().c_str());
            ImGui::EndDisabled();

            ImGui::SameLine();
            ImGui::Text("|");
            ImGui::SameLine();

            bool first = true;
            for (ICommand* cmd : registry.commands())
            {
                if (!first) ImGui::SameLine();
                first = false;
                ImGui::BeginDisabled(!cmd->enabled());
                if (ImGui::Button(cmd->label().c_str()))
                    history.execute(cmd);
                ImGui::EndDisabled();
            }
        }
        ImGui::End();

        // ---- Feature Tree --------------------------------------------------
        if (ImGui::Begin("Feature Tree"))
        {
            if (ImGui::TreeNodeEx("Model", ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (int i = 0; i < static_cast<int>(features.size()); ++i)
                {
                    const FeatureInfo& fi = features[i];
                    const bool sel = (i == selectedFeature);
                    ImGuiTreeNodeFlags flags =
                        ImGuiTreeNodeFlags_Leaf |
                        ImGuiTreeNodeFlags_NoTreePushOnOpen |
                        (sel ? ImGuiTreeNodeFlags_Selected : 0);
                    ImGui::TreeNodeEx(fi.name.c_str(), flags);
                    if (ImGui::IsItemClicked())
                        selectedFeature = sel ? -1 : i;  // toggle selection
                    ImGui::SameLine();
                    ImGui::TextDisabled("[%u tris]", fi.triangleCount);
                }
                if (features.empty())
                    ImGui::TextDisabled("(no features)");
                ImGui::TreePop();
            }
        }
        ImGui::End();

        // ---- Transform panel (selected feature) ----------------------------
        if (ImGui::Begin("Transform"))
        {
            if (selectedFeature >= 0 &&
                selectedFeature < static_cast<int>(transforms.size()))
            {
                FeatureTransform& xf = transforms[selectedFeature];
                ImGui::Text("Feature: %s", features[selectedFeature].name.c_str());
                ImGui::Separator();

                ImGui::TextUnformatted("Translation");
                bool c = false;
                c |= inputString("tx##T", xf.tx); ImGui::SameLine();
                c |= inputString("ty##T", xf.ty); ImGui::SameLine();
                c |= inputString("tz##T", xf.tz);
                ImGui::TextUnformatted("Rotation (deg)");
                c |= inputString("rx##T", xf.rx); ImGui::SameLine();
                c |= inputString("ry##T", xf.ry); ImGui::SameLine();
                c |= inputString("rz##T", xf.rz);
                if (c) dirty |= kDirtyTransforms;
                ImGui::Separator();
                ImGui::TextDisabled("Literals (\"2.5\") or param names (\"tx1\")");
            }
            else
            {
                ImGui::TextDisabled("(click a feature in the tree to edit its transform)");
            }
        }
        ImGui::End();

        // ---- Assembly constraints panel ------------------------------------
        if (ImGui::Begin("Assembly"))
        {
            const int nf = static_cast<int>(features.size());

            // ---- Add new constraint ----------------------------------------
            static int    newKind = 0;
            static int    newA = 0, newB = 0;
            static char   newVal[32] = "0";
            static char   newName[48] = "";

            const char* kindNames[] = {
                "Z-Stack","X-Distance","Y-Distance","Z-Distance",
                "Align X","Align Y","Align Z"
            };
            ImGui::SetNextItemWidth(100.0f);
            ImGui::Combo("Kind##nc", &newKind, kindNames, 7);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(50.0f);
            ImGui::InputInt("A##nc", &newA); newA = std::clamp(newA, 0, std::max(0, nf - 1));
            ImGui::SameLine();
            ImGui::SetNextItemWidth(50.0f);
            ImGui::InputInt("B##nc", &newB); newB = std::clamp(newB, 0, std::max(0, nf - 1));

            const bool needsVal = (newKind >= 1 && newKind <= 3); // *Distance
            if (needsVal) {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(60.0f);
                ImGui::InputText("val##nc", newVal, sizeof(newVal));
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(70.0f);
            ImGui::InputText("name##nc", newName, sizeof(newName));
            ImGui::SameLine();
            if (ImGui::Button("+##addc") && nf >= 2)
            {
                AsmConstraint c;
                c.kind     = static_cast<AsmConstraintKind>(newKind);
                c.featureA = newA;
                c.featureB = newB;
                c.value    = needsVal ? newVal : "0";
                c.name     = newName[0] ? newName
                                        : std::string(kindNames[newKind]) + " " +
                                          std::to_string(constraints.size() + 1);
                constraints.push_back(std::move(c));
                dirty |= kDirtyConstraints;
            }
            if (nf < 2)
                ImGui::TextDisabled("Need ≥ 2 features to add constraints.");

            ImGui::Separator();

            // ---- Existing constraints --------------------------------------
            int toDelete = -1;
            for (int i = 0; i < static_cast<int>(constraints.size()); ++i)
            {
                AsmConstraint& c = constraints[i];
                ImGui::PushID(i);

                char label[32];
                std::snprintf(label, sizeof(label), "x##dc%d", i);
                if (ImGui::SmallButton(label)) toDelete = i;
                ImGui::SameLine();

                ImGui::TextUnformatted(asmConstraintName(c.kind));
                ImGui::SameLine();

                const bool hasVal = (c.kind == AsmConstraintKind::XDistance ||
                                     c.kind == AsmConstraintKind::YDistance ||
                                     c.kind == AsmConstraintKind::ZDistance);
                if (hasVal)
                {
                    if (inputString("##cv", c.value, 60.0f))
                        dirty |= kDirtyConstraints;
                    ImGui::SameLine();
                }

                // Feature A / B pickers.
                int a = c.featureA, b = c.featureB;
                ImGui::SetNextItemWidth(40.0f);
                if (ImGui::InputInt("A##ci", &a)) {
                    c.featureA = std::clamp(a, 0, nf - 1);
                    dirty |= kDirtyConstraints;
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(40.0f);
                if (ImGui::InputInt("B##ci", &b)) {
                    c.featureB = std::clamp(b, 0, nf - 1);
                    dirty |= kDirtyConstraints;
                }
                ImGui::SameLine();
                ImGui::TextDisabled("  %s", c.name.c_str());

                ImGui::PopID();
            }
            if (toDelete >= 0)
            {
                constraints.erase(constraints.begin() + toDelete);
                dirty |= kDirtyConstraints;
            }
            if (constraints.empty())
                ImGui::TextDisabled("(no constraints)");

            ImGui::Separator();
            if (ImGui::Button("Solve Now") && !constraints.empty())
                dirty |= kDirtyConstraints;
        }
        ImGui::End();

        // ---- Parameters panel ----------------------------------------------
        if (ImGui::Begin("Parameters"))
        {
            static char newPName[64] = "";
            static double newPVal = 1.0;
            ImGui::SetNextItemWidth(90.0f);
            ImGui::InputText("##pname", newPName, sizeof(newPName));
            ImGui::SameLine();
            ImGui::SetNextItemWidth(70.0f);
            ImGui::InputDouble("##pval", &newPVal, 0.0, 0.0, "%.4g");
            ImGui::SameLine();
            if (ImGui::Button("+##addparam") && newPName[0] != '\0')
            {
                params.set(newPName, newPVal);
                newPName[0] = '\0';
                dirty |= kDirtyParams;
            }
            ImGui::Separator();

            std::string toDelParam;
            for (auto& [name, value] : params.all())
            {
                double editVal = value;
                char lbl[96];
                std::snprintf(lbl, sizeof(lbl), "##v_%s", name.c_str());
                ImGui::SetNextItemWidth(80.0f);
                if (ImGui::InputDouble(lbl, &editVal, 0.0, 0.0, "%.4g",
                                       ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    params.set(name, editVal);
                    dirty |= kDirtyParams;
                }
                ImGui::SameLine();
                ImGui::TextUnformatted(name.c_str());
                ImGui::SameLine();
                char dlbl[32];
                std::snprintf(dlbl, sizeof(dlbl), "x##d_%s", name.c_str());
                if (ImGui::SmallButton(dlbl)) toDelParam = name;
            }
            if (!toDelParam.empty())
            {
                params.remove(toDelParam);
                dirty |= kDirtyParams;
            }
            if (params.all().empty())
                ImGui::TextDisabled("(name + value, then +)");
        }
        ImGui::End();

        // ---- Stats ---------------------------------------------------------
        if (ImGui::Begin("Stats"))
        {
            ImGui::Text("Backend  : %s", stats.backend.c_str());
            ImGui::Text("FPS      : %.1f", stats.fps);
            ImGui::Text("Vertices : %u", stats.vertexCount);
            ImGui::Text("Triangles: %u", stats.triangleCount);
        }
        ImGui::End();

        // ---- Plugins (M6) --------------------------------------------------
        if (ImGui::Begin("Plugins"))
        {
            static char dllPath[256] = "macad_sample_plugin.dll";
            ImGui::SetNextItemWidth(200.0f);
            ImGui::InputText("##dllpath", dllPath, sizeof(dllPath));
            ImGui::SameLine();
            if (ImGui::Button("Load"))
            {
                if (registry.loadPluginLibrary(dllPath))
                    dirty |= kDirtyParams;  // toolbar gains new commands; force a redraw
            }
            ImGui::Separator();

            const auto names = registry.loadedPluginNames();
            if (names.empty())
                ImGui::TextDisabled("(no plugins loaded)");
            else
                for (const std::string& n : names)
                    ImGui::BulletText("%s", n.c_str());
        }
        ImGui::End();

        return dirty;
    }

} // namespace macad::ui
