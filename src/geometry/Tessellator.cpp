#include "geometry/Tessellator.hpp"

#include "core/Log.hpp"

#include <BRep_Tool.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <Poly_Triangulation.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>

#include <cstdint>

namespace macad::geometry {

    MeshData Tessellator::Tessellate(const Shape& shape, double deflection) {
        MeshData mesh;
        if (shape.isNull()) {
            MACAD_LOG_WARN("Tessellate called on null shape");
            return mesh;
        }

        const TopoDS_Shape& s = shape.occt();

        // Generate (or refresh) the triangulation stored on the faces.
        BRepMesh_IncrementalMesh mesher(s, deflection, /*isRelative*/ false,
            /*angDeflection*/ 0.5, /*parallel*/ true);
        mesher.Perform();

        std::size_t faceCount = 0;

        for (TopExp_Explorer exp(s, TopAbs_FACE); exp.More(); exp.Next()) {
            const TopoDS_Face face = TopoDS::Face(exp.Current());
            TopLoc_Location loc;
            Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
            if (tri.IsNull() || tri->NbTriangles() == 0) {
                continue;
            }
            ++faceCount;

            const gp_Trsf trsf = loc.Transformation();
            const bool reversed = (face.Orientation() == TopAbs_REVERSED);

            // Index offset where this face's vertices begin in the global arrays.
            const auto base = static_cast<std::uint32_t>(mesh.positions.size());

            // Copy and transform nodes; normals are accumulated below.
            const int nbNodes = tri->NbNodes();
            for (int i = 1; i <= nbNodes; ++i) {
                gp_Pnt p = tri->Node(i);
                p.Transform(trsf);
                mesh.positions.emplace_back(static_cast<float>(p.X()),
                    static_cast<float>(p.Y()),
                    static_cast<float>(p.Z()));
                mesh.normals.emplace_back(0.0f, 0.0f, 0.0f);
            }

            // Emit triangles with winding corrected for face orientation, and
            // accumulate geometric normals for smooth shading.
            const int nbTris = tri->NbTriangles();
            for (int t = 1; t <= nbTris; ++t) {
                int n1, n2, n3;
                tri->Triangle(t).Get(n1, n2, n3);
                if (reversed) {
                    std::swap(n2, n3);
                }
                const std::uint32_t i1 = base + static_cast<std::uint32_t>(n1 - 1);
                const std::uint32_t i2 = base + static_cast<std::uint32_t>(n2 - 1);
                const std::uint32_t i3 = base + static_cast<std::uint32_t>(n3 - 1);

                mesh.indices.push_back(i1);
                mesh.indices.push_back(i2);
                mesh.indices.push_back(i3);

                const vec3& p1 = mesh.positions[i1];
                const vec3& p2 = mesh.positions[i2];
                const vec3& p3 = mesh.positions[i3];
                const vec3 faceNormal = glm::cross(p2 - p1, p3 - p1);
                mesh.normals[i1] += faceNormal;
                mesh.normals[i2] += faceNormal;
                mesh.normals[i3] += faceNormal;
            }
        }

        // Normalize accumulated vertex normals.
        for (vec3& n : mesh.normals) {
            const float len = glm::length(n);
            n = (len > 1e-12f) ? n / len : vec3(0.0f, 0.0f, 1.0f);
        }

        MACAD_LOG_INFO("Tessellated {} faces -> {} verts, {} tris", faceCount,
            mesh.vertexCount(), mesh.triangleCount());
        return mesh;
    }

} // namespace macad::geometry
