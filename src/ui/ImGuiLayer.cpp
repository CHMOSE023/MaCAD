#include "ui/ImGuiLayer.hpp"

#include "core/Log.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_bgfx.h>

namespace macad::ui {

    namespace {
        constexpr std::uint16_t kImGuiViewId = 255;
    } // namespace

    ImGuiLayer::~ImGuiLayer() { shutdown(); }

    bool ImGuiLayer::init(GLFWwindow* window) {
        if (m_initialized) {
            return true;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO(); // ensure IO is initialized
        // NOTE: docking (ImGuiConfigFlags_DockingEnable) requires ImGui's docking
        // branch. We pin a stable release here; switch the FetchContent tag to a
        // docking build to enable dockable panels in a later milestone.
        ImGui::StyleColorsDark();

        // GLFW platform backend: we render via bgfx, so install callbacks but do
        // not let ImGui assume an OpenGL context.
        if (!ImGui_ImplGlfw_InitForOther(window, true)) {
            MACAD_LOG_ERROR("ImGui_ImplGlfw_InitForOther failed");
            return false;
        }
        if (!macad::ui_backend::ImGui_Implbgfx_Init(kImGuiViewId)) {
            MACAD_LOG_ERROR("ImGui_Implbgfx_Init failed");
            return false;
        }

        m_initialized = true;
        MACAD_LOG_INFO("ImGui layer initialized");
        return true;
    }

    void ImGuiLayer::shutdown() {
        if (!m_initialized) {
            return;
        }
        macad::ui_backend::ImGui_Implbgfx_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        m_initialized = false;
    }

    void ImGuiLayer::beginFrame() {
        macad::ui_backend::ImGui_Implbgfx_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::endFrame() {
        ImGui::Render();
        macad::ui_backend::ImGui_Implbgfx_RenderDrawData(ImGui::GetDrawData());
    }

} // namespace macad::ui
