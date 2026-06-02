#pragma once

// Minimal Dear ImGui rendering backend for bgfx.
//
// ImGui has official *platform* backends (we use imgui_impl_glfw) but no
// official bgfx *renderer* backend, so this vendored file provides one. It
// owns an ImGui shader program, the font atlas texture, and submits ImGui draw
// data through transient bgfx buffers on a dedicated view.
//
// Usage (per frame):
//   ImGui_ImplGlfw_NewFrame();
//   ImGui_Implbgfx_NewFrame();
//   ImGui::NewFrame();  ... build UI ...  ImGui::Render();
//   ImGui_Implbgfx_RenderDrawData(ImGui::GetDrawData());

#include <cstdint>

struct ImDrawData;

namespace macad::ui_backend {

// `viewId` is the bgfx view ImGui renders into (use a high id, e.g. 255).
bool ImGui_Implbgfx_Init(std::uint16_t viewId);
void ImGui_Implbgfx_Shutdown();
void ImGui_Implbgfx_NewFrame();
void ImGui_Implbgfx_RenderDrawData(ImDrawData* drawData);

} // namespace macad::ui_backend
