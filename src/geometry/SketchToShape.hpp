#pragma once

// Converts a macad::sketch::Sketch into OCCT B-Rep geometry.
// This is the only bridge between the sketch model and the solid modelling
// kernel; all OCCT headers are confined to the .cpp file.
//
// Dependency rule: geometry layer ONLY — do not include this from ui or render.

#include "geometry/Shape.hpp"
#include "sketch/Sketch.hpp"
#include "sketch/SketchPlane.hpp"

namespace macad::geometry {

    class SketchToShape {
    public:
        // Build a planar face from the sketch's closed profile(s).
        //   - Closed line/arc loops  → polygon / mixed-profile face
        //   - Circle entities        → circular face
        // Returns a null Shape if no valid closed profile is found.
        static Shape buildFace(const sketch::Sketch& sketch);

        // Extrude a face by `height` units along the sketch plane normal.
        // Negative height extrudes in the opposite direction.
        static Shape extrude(const Shape& face,
                             const sketch::SketchPlane& plane,
                             double height);

        // Revolve a face around one of the sketch plane's own axes.
        //   aroundV = true  → revolve around the plane's V axis (sketch "Y")
        //   aroundV = false → revolve around the plane's U axis (sketch "X")
        //   angleDeg        → sweep angle in degrees (360 = full solid of revolution)
        static Shape revolve(const Shape& face,
                             const sketch::SketchPlane& plane,
                             bool   aroundV,
                             double angleDeg);
    };

} // namespace macad::geometry
