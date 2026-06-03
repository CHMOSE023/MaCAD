#include "imgui_impl_bgfx.h"

#include <imgui.h>

#include <bgfx/bgfx.h>
#include <bx/math.h>

#include "dx11/vs_imgui.sc.bin.h"
#include "dx11/fs_imgui.sc.bin.h"

#include <cstring>

namespace macad::ui_backend {

namespace {

struct State {
    std::uint16_t viewId = 255;
    bgfx::VertexLayout layout;
    bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle fontTexture = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle texUniform = BGFX_INVALID_HANDLE;
};

State g_state;

void createFontsTexture() {
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels = nullptr;
    int width = 0, height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    g_state.fontTexture = bgfx::createTexture2D(
        static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height),
        false, 1, bgfx::TextureFormat::RGBA8, 0,
        bgfx::copy(pixels, static_cast<std::uint32_t>(width * height * 4)));

    // ImTextureID is an integer type (ImU64) in recent ImGui; a C-style cast
    // works whether it is an integer or a pointer.
    io.Fonts->SetTexID((ImTextureID)(uintptr_t)g_state.fontTexture.idx);
}

} // namespace

bool ImGui_Implbgfx_Init(std::uint16_t viewId) {
    g_state.viewId = viewId;

    g_state.layout.begin()
        .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();

    g_state.program = bgfx::createProgram(
        bgfx::createShader(bgfx::makeRef(vs_imgui_dx11, sizeof(vs_imgui_dx11))),
        bgfx::createShader(bgfx::makeRef(fs_imgui_dx11, sizeof(fs_imgui_dx11))),
        true);

    g_state.texUniform =
        bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

    createFontsTexture();
    return bgfx::isValid(g_state.program);
}

void ImGui_Implbgfx_Shutdown() {
    if (bgfx::isValid(g_state.fontTexture)) {
        bgfx::destroy(g_state.fontTexture);
    }
    if (bgfx::isValid(g_state.texUniform)) {
        bgfx::destroy(g_state.texUniform);
    }
    if (bgfx::isValid(g_state.program)) {
        bgfx::destroy(g_state.program);
    }
    g_state = State{};
}

void ImGui_Implbgfx_NewFrame() {
    if (!bgfx::isValid(g_state.fontTexture)) {
        createFontsTexture();
    }
}

void ImGui_Implbgfx_RenderDrawData(ImDrawData* drawData) {
    const ImGuiIO& io = ImGui::GetIO();
    const int fbWidth =
        static_cast<int>(drawData->DisplaySize.x * io.DisplayFramebufferScale.x);
    const int fbHeight =
        static_cast<int>(drawData->DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fbWidth <= 0 || fbHeight <= 0) {
        return;
    }

    const bgfx::Caps* caps = bgfx::getCaps();
    {
        const float x = drawData->DisplayPos.x;
        const float y = drawData->DisplayPos.y;
        const float w = drawData->DisplaySize.x;
        const float h = drawData->DisplaySize.y;
        float ortho[16];
        bx::mtxOrtho(ortho, x, x + w, y + h, y, 0.0f, 1000.0f, 0.0f,
                     caps->homogeneousDepth);
        bgfx::setViewTransform(g_state.viewId, nullptr, ortho);
        bgfx::setViewRect(g_state.viewId, 0, 0,
                          static_cast<std::uint16_t>(fbWidth),
                          static_cast<std::uint16_t>(fbHeight));
    }

    const ImVec2 clipOff = drawData->DisplayPos;
    const ImVec2 clipScale = io.DisplayFramebufferScale;

    for (int n = 0; n < drawData->CmdListsCount; ++n) {
        const ImDrawList* cmdList = drawData->CmdLists[n];

        const std::uint32_t numVertices =
            static_cast<std::uint32_t>(cmdList->VtxBuffer.Size);
        const std::uint32_t numIndices =
            static_cast<std::uint32_t>(cmdList->IdxBuffer.Size);

        if (bgfx::getAvailTransientVertexBuffer(numVertices, g_state.layout) <
                numVertices ||
            bgfx::getAvailTransientIndexBuffer(numIndices) < numIndices) {
            break; // not enough transient space this frame
        }

        bgfx::TransientVertexBuffer tvb;
        bgfx::TransientIndexBuffer tib;
        bgfx::allocTransientVertexBuffer(&tvb, numVertices, g_state.layout);
        bgfx::allocTransientIndexBuffer(&tib, numIndices,
                                        sizeof(ImDrawIdx) == 4);

        std::memcpy(tvb.data, cmdList->VtxBuffer.Data,
                    numVertices * sizeof(ImDrawVert));
        std::memcpy(tib.data, cmdList->IdxBuffer.Data,
                    numIndices * sizeof(ImDrawIdx));

        for (int cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; ++cmdIdx) {
            const ImDrawCmd& cmd = cmdList->CmdBuffer[cmdIdx];
            if (cmd.UserCallback) {
                cmd.UserCallback(cmdList, &cmd);
                continue;
            }
            if (cmd.ElemCount == 0) {
                continue;
            }

            const float clipX = (cmd.ClipRect.x - clipOff.x) * clipScale.x;
            const float clipY = (cmd.ClipRect.y - clipOff.y) * clipScale.y;
            const float clipZ = (cmd.ClipRect.z - clipOff.x) * clipScale.x;
            const float clipW = (cmd.ClipRect.w - clipOff.y) * clipScale.y;

            const std::uint16_t sx =
                static_cast<std::uint16_t>(clipX < 0 ? 0 : clipX);
            const std::uint16_t sy =
                static_cast<std::uint16_t>(clipY < 0 ? 0 : clipY);
            bgfx::setScissor(sx, sy,
                             static_cast<std::uint16_t>(clipZ - clipX),
                             static_cast<std::uint16_t>(clipW - clipY));

            bgfx::TextureHandle tex{(std::uint16_t)(uintptr_t)cmd.GetTexID()};
            bgfx::setTexture(0, g_state.texUniform, tex);

            bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                           BGFX_STATE_MSAA |
                           BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA,
                                                 BGFX_STATE_BLEND_INV_SRC_ALPHA));

            bgfx::setVertexBuffer(0, &tvb, cmd.VtxOffset, numVertices);
            bgfx::setIndexBuffer(&tib, cmd.IdxOffset, cmd.ElemCount);
            bgfx::submit(g_state.viewId, g_state.program);
        }
    }
}

} // namespace macad::ui_backend
