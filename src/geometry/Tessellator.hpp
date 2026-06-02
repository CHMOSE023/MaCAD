#pragma once

// Converts a B-Rep Shape into a renderable triangle mesh (core::MeshData).
// This is the bridge from the OCCT world to the bgfx world; the output type
// contains no OCCT references.

#include "core/MeshData.hpp"
#include "geometry/Shape.hpp"

namespace macad::geometry {

    class Tessellator {
    public:
        // Triangulates `shape` with the given linear deflection (smaller = finer)
        // and returns interleaved positions/normals/indices. Normals are computed
        // per-vertex from the surface where available, else from face geometry.
        static MeshData Tessellate(const Shape& shape, double deflection = 0.5);
    };

} // namespace macad::geometry
