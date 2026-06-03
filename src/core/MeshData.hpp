#pragma once

// The single data contract between the geometry layer (OCCT) and the render
// layer (bgfx). Any TopoDS_Shape produced by sketches/features/assemblies is
// tessellated into MeshData, which the renderer uploads. This keeps OCCT
// types out of render/ui entirely.

#include <cstdint>
#include <limits>
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

        // Axis-aligned bounding box in local (pre-transform) space.
        // Returns (min, max); both are {0,0,0} for an empty mesh.
        std::pair<vec3, vec3> computeAABB() const {
            if (positions.empty())
                return { vec3(0.0f), vec3(0.0f) };
            constexpr float kInf = std::numeric_limits<float>::max();
            vec3 mn( kInf), mx(-kInf);
            for (const vec3& p : positions) {
                mn = glm::min(mn, p);
                mx = glm::max(mx, p);
            }
            return { mn, mx };
        }
    };

} // namespace macad
