#include "render/Mesh.hpp"

#include "core/Log.hpp"

#include <bgfx/bgfx.h>

#include <vector>

namespace macad::render {

    namespace {

        struct Vertex {
            float px, py, pz;
            float nx, ny, nz;
        };

        const bgfx::VertexLayout& vertexLayout() {
            static bgfx::VertexLayout layout = [] {
                bgfx::VertexLayout l;
                l.begin()
                    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                    .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
                    .end();
                return l;
                }();
            return layout;
        }

    } // namespace

    Mesh::~Mesh() { destroy(); }

    void Mesh::destroy() 
    {
        if (m_vbh != bgfx::kInvalidHandle) 
        {
            bgfx::destroy(bgfx::VertexBufferHandle{ m_vbh });
            m_vbh = bgfx::kInvalidHandle;
        }
        if (m_ibh != bgfx::kInvalidHandle) 
        {
            bgfx::destroy(bgfx::IndexBufferHandle{ m_ibh });
            m_ibh = bgfx::kInvalidHandle;
        }
        m_indexCount = 0;
        m_valid = false;
    }

    void Mesh::upload(const MeshData& mesh) {
        destroy();
        if (mesh.empty()) {
            MACAD_LOG_WARN("Mesh::upload called with empty mesh");
            return;
        }

        // Interleave positions + normals.
        std::vector<Vertex> verts(mesh.vertexCount());
        for (std::size_t i = 0; i < mesh.vertexCount(); ++i) {
            const vec3& p = mesh.positions[i];
            const vec3& n = i < mesh.normals.size() ? mesh.normals[i]
                : vec3(0.0f, 0.0f, 1.0f);
            verts[i] = { p.x, p.y, p.z, n.x, n.y, n.z };
        }

        const bgfx::Memory* vmem =
            bgfx::copy(verts.data(), static_cast<std::uint32_t>(verts.size() * sizeof(Vertex)));
        const bgfx::Memory* imem =
            bgfx::copy(mesh.indices.data(),
                static_cast<std::uint32_t>(mesh.indices.size() * sizeof(std::uint32_t)));

        m_vbh = bgfx::createVertexBuffer(vmem, vertexLayout()).idx;
        m_ibh = bgfx::createIndexBuffer(imem, BGFX_BUFFER_INDEX32).idx;
        m_indexCount = static_cast<std::uint32_t>(mesh.indices.size());
        m_valid = true;
    }

    void Mesh::setBuffers() const {
        if (!m_valid) {
            return;
        }
        bgfx::setVertexBuffer(0, bgfx::VertexBufferHandle{ m_vbh });
        bgfx::setIndexBuffer(bgfx::IndexBufferHandle{ m_ibh });
    }

} // namespace macad::render
