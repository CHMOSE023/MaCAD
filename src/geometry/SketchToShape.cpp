#include "geometry/SketchToShape.hpp"

#include "core/Log.hpp"

// OCCT B-Rep construction
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
// OCCT geometry primitives
#include <GC_MakeArcOfCircle.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <vector>

namespace macad::geometry {

    namespace {

        // Convert a sketch 2D point to an OCCT 3D point using the sketch plane.
        gp_Pnt toPnt(const sketch::SketchPlane& pl, const glm::vec2& uv) {
            const glm::vec3 w = pl.to3d(uv);
            return { w.x, w.y, w.z };
        }

        // Build a face from all live Line entities (and Arc entities) in the
        // sketch.  BRepBuilderAPI_MakeWire can sort unordered edges, so we just
        // feed it every edge and let OCCT figure out the connectivity.
        // Returns a null shape if no closed wire can be formed.
        Shape tryLineArcFace(const sketch::Sketch& sketch) {
            const sketch::SketchPlane& pl = sketch.plane();
            BRepBuilderAPI_MakeWire wireBuilder;
            int edgeCount = 0;

            for (const sketch::Entity& e : sketch.entities()) {
                if (e.removed) continue;

                if (e.kind == sketch::EntityKind::Line) {
                    const gp_Pnt p0 = toPnt(pl, sketch.pointPos(e.p0));
                    const gp_Pnt p1 = toPnt(pl, sketch.pointPos(e.p1));
                    // Skip degenerate edges (duplicate points).
                    if (p0.Distance(p1) < 1e-7) continue;
                    BRepBuilderAPI_MakeEdge edge(p0, p1);
                    if (!edge.IsDone()) continue;
                    wireBuilder.Add(edge.Edge());
                    ++edgeCount;
                }
                else if (e.kind == sketch::EntityKind::Arc) {
                    const gp_Pnt center = toPnt(pl, sketch.pointPos(e.p0));
                    const double r      = sketch.radius(e.id);
                    const double aStart = sketch.arcStart(e.id);
                    const double aEnd   = sketch.arcEnd(e.id);

                    // Arc start / end points in 3D.
                    const glm::vec2 uvS{
                        static_cast<float>(sketch.pointPos(e.p0).x + r * std::cos(aStart)),
                        static_cast<float>(sketch.pointPos(e.p0).y + r * std::sin(aStart)) };
                    const glm::vec2 uvE{
                        static_cast<float>(sketch.pointPos(e.p0).x + r * std::cos(aEnd)),
                        static_cast<float>(sketch.pointPos(e.p0).y + r * std::sin(aEnd)) };
                    // Mid-angle for the three-point constructor.
                    const double aMid = aStart + (aEnd - aStart) * 0.5;
                    const glm::vec2 uvM{
                        static_cast<float>(sketch.pointPos(e.p0).x + r * std::cos(aMid)),
                        static_cast<float>(sketch.pointPos(e.p0).y + r * std::sin(aMid)) };

                    const Handle(Geom_TrimmedCurve) arc =
                        GC_MakeArcOfCircle(toPnt(pl, uvS), toPnt(pl, uvM), toPnt(pl, uvE));
                    if (arc.IsNull()) continue;
                    BRepBuilderAPI_MakeEdge edge(arc);
                    if (!edge.IsDone()) continue;
                    wireBuilder.Add(edge.Edge());
                    ++edgeCount;
                }
            }

            if (edgeCount == 0) return {};
            if (!wireBuilder.IsDone()) {
                MACAD_LOG_WARN("SketchToShape: could not build a closed wire from {} edges "
                               "(profile not closed?)", edgeCount);
                return {};
            }

            BRepBuilderAPI_MakeFace faceBuilder(wireBuilder.Wire(), /*planar=*/Standard_True);
            if (!faceBuilder.IsDone()) {
                MACAD_LOG_WARN("SketchToShape: wire is not planar or not closed");
                return {};
            }
            return Shape(faceBuilder.Face());
        }

        // Build a face from a single Circle entity.
        Shape tryCircleFace(const sketch::Sketch& sketch,
                            const sketch::Entity& e) {
            const sketch::SketchPlane& pl = sketch.plane();
            const glm::vec3 n             = pl.normal();
            const gp_Pnt    center        = toPnt(pl, sketch.pointPos(e.p0));
            const double    r             = sketch.radius(e.id);
            if (r < 1e-7) return {};

            const gp_Ax2 ax(center, gp_Dir(n.x, n.y, n.z));
            BRepBuilderAPI_MakeEdge edge(gp_Circ(ax, r));
            if (!edge.IsDone()) return {};
            BRepBuilderAPI_MakeWire wire(edge.Edge());
            if (!wire.IsDone()) return {};
            BRepBuilderAPI_MakeFace face(wire.Wire(), Standard_True);
            if (!face.IsDone()) return {};
            return Shape(face.Face());
        }

    } // namespace

    // -------------------------------------------------------------------------

    Shape SketchToShape::buildFace(const sketch::Sketch& sketch) {
        // 1. Try to build a face from the line/arc network first.
        bool hasLines = false;
        for (const sketch::Entity& e : sketch.entities()) {
            if (!e.removed &&
               (e.kind == sketch::EntityKind::Line ||
                e.kind == sketch::EntityKind::Arc)) {
                hasLines = true;
                break;
            }
        }
        if (hasLines) {
            Shape s = tryLineArcFace(sketch);
            if (!s.isNull()) return s;
        }

        // 2. Fall back: return the first valid circle face.
        for (const sketch::Entity& e : sketch.entities()) {
            if (!e.removed && e.kind == sketch::EntityKind::Circle) {
                Shape s = tryCircleFace(sketch, e);
                if (!s.isNull()) return s;
            }
        }

        MACAD_LOG_WARN("SketchToShape::buildFace: no closed profile found in sketch");
        return {};
    }

    Shape SketchToShape::extrude(const Shape& face,
                                 const sketch::SketchPlane& plane,
                                 double height) {
        if (face.isNull()) return {};
        const glm::vec3 n = plane.normal();
        const gp_Vec dir(n.x * height, n.y * height, n.z * height);
        BRepPrimAPI_MakePrism prism(face.occt(), dir);
        if (!prism.IsDone()) {
            MACAD_LOG_ERROR("SketchToShape::extrude: BRepPrimAPI_MakePrism failed");
            return {};
        }
        return Shape(prism.Shape());
    }

} // namespace macad::geometry
