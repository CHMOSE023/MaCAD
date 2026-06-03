#include "ui/Panels.hpp"

#include "plugin/PluginRegistry.hpp"
#include "plugin/CommandStack.hpp"
#include "plugin/ICommand.hpp"

#include <imgui.h>

namespace macad::ui 
{

    void Panels::draw(PluginRegistry& registry, CommandStack& history, const FrameStats& stats, const std::vector<FeatureInfo>& features)
    {
        // ---- Toolbar (top) -----------------------------------------------------
        if (ImGui::Begin("Toolbar")) 
        {
            // Undo / Redo buttons with tooltip showing the target action label.
            const bool canUndo = history.canUndo();
            const bool canRedo = history.canRedo();
            ImGui::BeginDisabled(!canUndo);
            if (ImGui::Button("Undo")) { history.undo(); }
            if (canUndo && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("Undo: %s", history.undoLabel().c_str());
            }
            ImGui::EndDisabled();

            ImGui::SameLine();

            ImGui::BeginDisabled(!canRedo);
            if (ImGui::Button("Redo")) { history.redo(); }
            if (canRedo && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("Redo: %s", history.redoLabel().c_str());
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            ImGui::Text("|");
            ImGui::SameLine();

            // Registered commands — routed through the CommandStack.
            bool first = true;

            for (ICommand* cmd : registry.commands()) 
            {
                if (!first)
                {
                    ImGui::SameLine();
                }
                first = false;
                ImGui::BeginDisabled(!cmd->enabled());
                if (ImGui::Button(cmd->label().c_str())) 
                {
                    history.execute(cmd);
                }
                ImGui::EndDisabled();
            }

            if (first) 
            {
                ImGui::TextUnformatted("No commands registered");
            }
        }
        ImGui::End();

        // ---- Feature tree --------------------------------------------------
        if (ImGui::Begin("Feature Tree"))
        {
            if (ImGui::TreeNodeEx("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (features.empty()) {
                    ImGui::TextDisabled("(no features yet)");
                } else {
                    for (const FeatureInfo& f : features) {
                        ImGui::BulletText("%s  [%u tris]", f.name.c_str(), f.triangleCount);
                    }
                }
                ImGui::TreePop();
            }
        }
        ImGui::End();

        // ---- Stats overlay (bottom-right) --------------------------------------
        if (ImGui::Begin("Stats")) 
        {
            ImGui::Text("Backend : %s", stats.backend.c_str());
            ImGui::Text("FPS     : %.1f", stats.fps);
            ImGui::Text("Vertices: %u", stats.vertexCount);
            ImGui::Text("Triangles: %u", stats.triangleCount);
        }
        ImGui::End();
    }

} // namespace macad::ui
