#pragma once

// The 2D parametric sketch model: points, geometry (line/circle/arc) and
// constraints. Pure data + glm math, no OCCT and no rendering -- the solver runs
// against the flat variable array owned here, and the UI layer renders/edits it
// through this interface.
//
// Variable model: every scalar degree of freedom (point x/y, circle radius, arc
// angles) is a slot in m_vars. Entities and constraints reference those slots by
// index, which lets the generic solver (see Solver) treat the whole sketch as a
// vector of unknowns without knowing entity specifics.

#include "core/Types.hpp"
#include "sketch/SketchPlane.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace macad::sketch
{

    struct PointTag      {};
    struct EntityTag     {};
    struct ConstraintTag {};

    using  PointId       = StrongId<PointTag>;
    using  EntityId      = StrongId<EntityTag>;
    using  ConstraintId  = StrongId<ConstraintTag>;

    enum class EntityKind { Point, Line, Circle, Arc };

    enum class ConstraintKind
    {
        // Geometric (no value).
        Coincident,   // point == point
        Horizontal,   // line is horizontal
        Vertical,     // line is vertical
        Parallel,     // line || line
        Tangent,      // line~circle or circle~circle
        Equal,        // equal length (line/line) or equal radius (circle/circle)
        // Dimensional (driven by `value`).
        Distance,     // |p0 - p1| == value
        Radius,       // entity radius == value
        Angle,        // angle(line0, line1) == value (radians)
    };

    bool isDimension(ConstraintKind k);
    const char* constraintName(ConstraintKind k);

    struct Point
    {
        PointId id;
        int vx{ -1 };       // index into Sketch::m_vars
        int vy{ -1 };
        bool fixed{ false }; // anchored: solver never moves it (e.g. origin, drag target)
        bool removed{ false };
    };

    struct Entity 
    {
        EntityId   id;
        EntityKind kind{ EntityKind::Point };
        PointId    p0;                    // point / line-start / circle-center / arc-center
        PointId    p1;                    // line-end (Line only)
        int        vr     { -1 };         // radius var (Circle, Arc)
        int        vStart { -1 };         // arc start angle var (radians)
        int        vEnd   { -1 };         // arc end angle var (radians)
        bool       removed{ false };
    };

    struct Constraint
    {
        ConstraintId   id;
        ConstraintKind kind{ ConstraintKind::Coincident };
        EntityId       e0;
        EntityId       e1;
        PointId        p0;
        PointId        p1;
        double         value{ 0.0 };  // dimensional target
        bool           removed{ false };
    };

    class Sketch
    {
    public:
        Sketch() = default;

        const SketchPlane& plane() const { return m_plane; }
        void setPlane(const SketchPlane& p) { m_plane = p; }

        // ---- construction --------------------------------------------------
        PointId  addPoint(double x, double y, bool fixed = false);
        EntityId addPointEntity(double x, double y);
        EntityId addLine(double x0, double y0, double x1, double y1);
        EntityId addLineFromPoints(PointId a, PointId b);
        EntityId addCircle(double cx, double cy, double r);
        EntityId addArc(double cx, double cy, double r, double startAngle, double endAngle);

        ConstraintId addConstraint(const Constraint& c);
        // Convenience builders (validate operand arity; return invalid id on misuse).
        ConstraintId addCoincident(PointId a, PointId b);
        ConstraintId addGeometric(ConstraintKind kind, EntityId e0, EntityId e1 = {});
        ConstraintId addDistance(PointId a, PointId b, double value);
        ConstraintId addRadius(EntityId circle, double value);
        ConstraintId addAngle(EntityId line0, EntityId line1, double value);

        void removeEntity(EntityId id);
        void removeConstraint(ConstraintId id);
        void clear();

        // ---- access --------------------------------------------------------
        const std::vector<Point>&      points()      const { return m_points; }
        const std::vector<Entity>&     entities()    const { return m_entities; }
        const std::vector<Constraint>& constraints() const { return m_constraints; }
        std::vector<Constraint>&       constraints()       { return m_constraints; }

        const Point& point(PointId id)    const { return m_points[id.value - 1]; }
        const Entity& entity(EntityId id) const { return m_entities[id.value - 1]; }
        bool valid(PointId id)            const { return id.value >= 1 && id.value <= m_points.size(); }
        bool valid(EntityId id)           const { return id.value >= 1 && id.value <= m_entities.size(); }

        vec2 pointPos(PointId id) const;
        void setPointPos(PointId id, const vec2& p);
        void setPointFixed(PointId id, bool fixed);

        double radius  (EntityId id) const;
        double arcStart(EntityId id) const;
        double arcEnd  (EntityId id) const;

        // ---- raw variable array (for the solver) ---------------------------
        std::size_t varCount() const { return m_vars.size(); }
        double var(int i) const { return m_vars[i]; }
        void setVar(int i, double v) { m_vars[i] = v; }

    private:
        int addVar(double v);

        SketchPlane m_plane;                     // world XY by default
        std::vector<double>     m_vars;          // flat DOF array
        std::vector<Point>      m_points;
        std::vector<Entity>     m_entities;
        std::vector<Constraint> m_constraints;
    };

} 
