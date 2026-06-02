#include "geometry/Primitives.hpp"

#include <BRepPrimAPI_MakeBox.hxx>
#include <TopoDS_Shape.hxx>

namespace macad::geometry {

    Shape Primitives::MakeBox(double dx, double dy, double dz) {
        BRepPrimAPI_MakeBox maker(dx, dy, dz);
        const TopoDS_Shape& solid = maker.Shape();
        return Shape(solid);
    }

} // namespace macad::geometry
