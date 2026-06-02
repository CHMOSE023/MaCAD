#pragma once

// Primitive solid construction wrappers over BRepPrimAPI_*. The public surface
// uses only the OCCT-free Shape wrapper, so callers (e.g. commands in the app
// layer) need no OCCT headers.

#include "geometry/Shape.hpp"

namespace macad::geometry {

    class Primitives {
    public:
        // Axis-aligned box of size dx*dy*dz with a corner at the origin.
        static Shape MakeBox(double dx, double dy, double dz);
    };

} // namespace macad::geometry
