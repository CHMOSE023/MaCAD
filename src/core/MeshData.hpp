#pragma once

// The single data contract between the geometry layer (OCCT) and the render
// layer (bgfx). Any TopoDS_Shape produced by sketches/features/assemblies is
// tessellated into MeshData, which the renderer uploads. This keeps OCCT
// types out of render/ui entirely.

#include <cstdint>
#include <vector>

#include "core/Types.hpp"

namespace macad {

    struct MeshData {
        std::vector<vec3> positions;
        std::vector<vec3> normals;
        std::vector<std::uint32_t> indices; // triangle list

        bool empty() const { return positions.empty() || indices.empty(); }
        std::size_t vertexCount() const { return positions.size(); }
        std::size_t triangleCount() const { return indices.size() / 3; }

        void clear() {
            positions.clear();
            normals.clear();
            indices.clear();
        }
    };

} // namespace macad
