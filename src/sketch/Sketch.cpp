#include "sketch/Sketch.hpp"

#include <cmath>

namespace macad::sketch {

    bool isDimension(ConstraintKind k) {
        return k == ConstraintKind::Distance || k == ConstraintKind::Radius ||
            k == ConstraintKind::Angle;
    }

    const char* constraintName(ConstraintKind k) {
        switch (k) {
        case ConstraintKind::Coincident: return "Coincident";
        case ConstraintKind::Horizontal: return "Horizontal";
        case ConstraintKind::Vertical:   return "Vertical";
        case ConstraintKind::Parallel:   return "Parallel";
        case ConstraintKind::Tangent:    return "Tangent";
        case ConstraintKind::Equal:      return "Equal";
        case ConstraintKind::Distance:   return "Distance";
        case ConstraintKind::Radius:     return "Radius";
        case ConstraintKind::Angle:      return "Angle";
        }
        return "?";
    }

    int Sketch::addVar(double v) {
        m_vars.push_back(v);
        return static_cast<int>(m_vars.size()) - 1;
    }

    PointId Sketch::addPoint(double x, double y, bool fixed) {
        Point p;
        p.id = PointId{ m_points.size() + 1 };
        p.vx = addVar(x);
        p.vy = addVar(y);
        p.fixed = fixed;
        m_points.push_back(p);
        return p.id;
    }

    EntityId Sketch::addPointEntity(double x, double y) {
        const PointId pid = addPoint(x, y);
        Entity e;
        e.id = EntityId{ m_entities.size() + 1 };
        e.kind = EntityKind::Point;
        e.p0 = pid;
        m_entities.push_back(e);
        return e.id;
    }

    EntityId Sketch::addLine(double x0, double y0, double x1, double y1) {
        return addLineFromPoints(addPoint(x0, y0), addPoint(x1, y1));
    }

    EntityId Sketch::addLineFromPoints(PointId a, PointId b) {
        Entity e;
        e.id = EntityId{ m_entities.size() + 1 };
        e.kind = EntityKind::Line;
        e.p0 = a;
        e.p1 = b;
        m_entities.push_back(e);
        return e.id;
    }

    EntityId Sketch::addCircle(double cx, double cy, double r) {
        Entity e;
        e.id = EntityId{ m_entities.size() + 1 };
        e.kind = EntityKind::Circle;
        e.p0 = addPoint(cx, cy);
        e.vr = addVar(r);
        m_entities.push_back(e);
        return e.id;
    }

    EntityId Sketch::addArc(double cx, double cy, double r, double startAngle, double endAngle) {
        Entity e;
        e.id = EntityId{ m_entities.size() + 1 };
        e.kind = EntityKind::Arc;
        e.p0 = addPoint(cx, cy);
        e.vr = addVar(r);
        e.vStart = addVar(startAngle);
        e.vEnd = addVar(endAngle);
        m_entities.push_back(e);
        return e.id;
    }

    ConstraintId Sketch::addConstraint(const Constraint& c) {
        Constraint copy = c;
        copy.id = ConstraintId{ m_constraints.size() + 1 };
        m_constraints.push_back(copy);
        return copy.id;
    }

    ConstraintId Sketch::addCoincident(PointId a, PointId b) {
        if (!valid(a) || !valid(b) || a == b) return {};
        Constraint c;
        c.kind = ConstraintKind::Coincident;
        c.p0 = a;
        c.p1 = b;
        return addConstraint(c);
    }

    ConstraintId Sketch::addGeometric(ConstraintKind kind, EntityId e0, EntityId e1) {
        if (isDimension(kind) || kind == ConstraintKind::Coincident) return {};
        if (!valid(e0)) return {};
        Constraint c;
        c.kind = kind;
        c.e0 = e0;
        c.e1 = e1;
        return addConstraint(c);
    }

    ConstraintId Sketch::addDistance(PointId a, PointId b, double value) {
        if (!valid(a) || !valid(b)) return {};
        Constraint c;
        c.kind = ConstraintKind::Distance;
        c.p0 = a;
        c.p1 = b;
        c.value = value;
        return addConstraint(c);
    }

    ConstraintId Sketch::addRadius(EntityId circle, double value) {
        if (!valid(circle)) return {};
        Constraint c;
        c.kind = ConstraintKind::Radius;
        c.e0 = circle;
        c.value = value;
        return addConstraint(c);
    }

    ConstraintId Sketch::addAngle(EntityId line0, EntityId line1, double value) {
        if (!valid(line0) || !valid(line1)) return {};
        Constraint c;
        c.kind = ConstraintKind::Angle;
        c.e0 = line0;
        c.e1 = line1;
        c.value = value;
        return addConstraint(c);
    }

    void Sketch::removeEntity(EntityId id) {
        if (!valid(id)) return;
        Entity& e = m_entities[id.value - 1];
        e.removed = true;

        // Tombstone the points this entity owns and pin them so the solver skips
        // their vars. Points are never shared between entities (coincidence is a
        // constraint, not a merge), so this is safe.
        auto kill = [this](PointId p) {
            if (valid(p)) {
                m_points[p.value - 1].removed = true;
                m_points[p.value - 1].fixed = true;
            }
        };
        kill(e.p0);
        if (e.kind == EntityKind::Line) kill(e.p1);

        // Drop constraints that reference the removed entity or its points.
        for (Constraint& c : m_constraints) {
            if (c.removed) continue;
            const bool touches =
                c.e0 == id || c.e1 == id ||
                (valid(c.p0) && (c.p0 == e.p0 || c.p0 == e.p1)) ||
                (valid(c.p1) && (c.p1 == e.p0 || c.p1 == e.p1));
            if (touches) c.removed = true;
        }
    }

    void Sketch::removeConstraint(ConstraintId id) {
        if (id.value >= 1 && id.value <= m_constraints.size()) {
            m_constraints[id.value - 1].removed = true;
        }
    }

    void Sketch::clear() {
        m_vars.clear();
        m_points.clear();
        m_entities.clear();
        m_constraints.clear();
    }

    vec2 Sketch::pointPos(PointId id) const {
        const Point& p = m_points[id.value - 1];
        return { static_cast<float>(m_vars[p.vx]), static_cast<float>(m_vars[p.vy]) };
    }

    void Sketch::setPointPos(PointId id, const vec2& v) {
        const Point& p = m_points[id.value - 1];
        m_vars[p.vx] = v.x;
        m_vars[p.vy] = v.y;
    }

    void Sketch::setPointFixed(PointId id, bool fixed) {
        if (valid(id)) m_points[id.value - 1].fixed = fixed;
    }

    double Sketch::radius(EntityId id) const {
        const Entity& e = m_entities[id.value - 1];
        return e.vr >= 0 ? m_vars[e.vr] : 0.0;
    }

    double Sketch::arcStart(EntityId id) const {
        const Entity& e = m_entities[id.value - 1];
        return e.vStart >= 0 ? m_vars[e.vStart] : 0.0;
    }

    double Sketch::arcEnd(EntityId id) const {
        const Entity& e = m_entities[id.value - 1];
        return e.vEnd >= 0 ? m_vars[e.vEnd] : 0.0;
    }

} // namespace macad::sketch
