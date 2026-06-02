#pragma once

// GPU mesh: owns bgfx vertex/index buffers built from a core::MeshData.
// Non-copyable; destroys its bgfx handles on destruction or re-upload.

#include "core/MeshData.hpp"

#include <cstdint>

namespace macad::render {

    class Mesh {
    public:
        Mesh() = default;
        ~Mesh();

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        // Replaces any existing GPU buffers with data from `mesh`.
        void upload(const MeshData& mesh);

        bool valid() const { return m_valid; }
        std::uint32_t indexCount() const { return m_indexCount; }

        // Binds the buffers to the current bgfx draw call (does not submit).
        void setBuffers() const;

        // Releases the bgfx buffers. Must be called while the bgfx context is
        // still alive (i.e. before bgfx::shutdown); the destructor also calls it.
        void destroy();

    private:

        std::uint16_t m_vbh{ 0xffff }; // bgfx::VertexBufferHandle idx
        std::uint16_t m_ibh{ 0xffff }; // bgfx::IndexBufferHandle idx
        std::uint32_t m_indexCount{ 0 };
        bool m_valid{ false };
    };

} // namespace macad::render
