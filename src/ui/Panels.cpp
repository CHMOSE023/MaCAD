#include "ui/Panels.hpp"

#include "plugin/PluginRegistry.hpp"
#include "plugin/ICommand.hpp"

#include <imgui.h>

namespace macad::ui {

    void Panels::draw(PluginRegistry& registry, const FrameStats& stats) {
        // ---- Toolbar (top) -----------------------------------------------------
        if (ImGui::Begin("Toolbar")) {
            bool first = true;
            for (ICommand* cmd : registry.commands()) {
                if (!first) {
                    ImGui::SameLine();
                }
                first = false;
                ImGui::BeginDisabled(!cmd->enabled());
                if (ImGui::Button(cmd->label().c_str())) {
                    cmd->execute();
                }
                ImGui::EndDisabled();
            }
            if (first) {
                ImGui::TextUnformatted("No commands registered");
            }
        }
        ImGui::End();

        // ---- Feature tree (left, placeholder) ----------------------------------
        if (ImGui::Begin("Feature Tree")) {
            ImGui::TextDisabled("(parametric feature tree — M3)");
            if (ImGui::TreeNodeEx("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::BulletText("Box");
                ImGui::TreePop();
            }
        }
        ImGui::End();

        // ---- Stats overlay (bottom-right) --------------------------------------
        if (ImGui::Begin("Stats")) {
            ImGui::Text("Backend : %s", stats.backend.c_str());
            ImGui::Text("FPS     : %.1f", stats.fps);
            ImGui::Text("Vertices: %u", stats.vertexCount);
            ImGui::Text("Triangles: %u", stats.triangleCount);
        }
        ImGui::End();
    }

} // namespace macad::ui
