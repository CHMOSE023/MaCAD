#include "ui/SketchView.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace macad::ui {

    using namespace macad::sketch;

    namespace {

        constexpr float kPi = 3.14159265358979323846f;
        constexpr float kPickPixels = 8.0f;

        const ImU32 kColGeom = IM_COL32(225, 225, 235, 255);
        const ImU32 kColSel = IM_COL32(255, 165, 40, 255);
        const ImU32 kColPoint = IM_COL32(120, 180, 255, 255);
        const ImU32 kColFixed = IM_COL32(255, 90, 90, 255);
        const ImU32 kColPreview = IM_COL32(150, 200, 255, 170);
        const ImU32 kColDim = IM_COL32(170, 255, 170, 255);
        const ImU32 kColAxis = IM_COL32(90, 90, 110, 200);

        float distToSegment(const ImVec2& p, const ImVec2& a, const ImVec2& b) {
            const float vx = b.x - a.x, vy = b.y - a.y;
            const float wx = p.x - a.x, wy = p.y - a.y;
            const float len2 = vx * vx + vy * vy;
            float t = len2 > 1e-6f ? (wx * vx + wy * vy) / len2 : 0.0f;
            t = std::clamp(t, 0.0f, 1.0f);
            const float dx = p.x - (a.x + t * vx), dy = p.y - (a.y + t * vy);
            return std::sqrt(dx * dx + dy * dy);
        }

        float dist(const ImVec2& a, const ImVec2& b) {
            return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
        }

    } // namespace

    // ---- lifecycle ---------------------------------------------------------

    void SketchView::toggle() {
        if (m_active) { end(); return; }
        m_sketch.clear();
        // Anchored origin so the sketch is grounded (removes 2 DOF) and there is a
        // visible reference. Drawing snaps to it.
        m_sketch.addPoint(0.0, 0.0, /*fixed*/ true);
        m_active = true;
        m_tool = Tool::Select;
        m_drawing = false;
        m_clickStage = 0;
        clearSelection();
        resolve();
    }

    void SketchView::end() {
        m_active = false;
        m_drawing = false;
        m_clickStage = 0;
        clearSelection();
    }

    // ---- projection --------------------------------------------------------

    bool SketchView::worldToScreen(const mat4& vp, const vec3& world, vec2& out) const {
        const ImVec2 disp = ImGui::GetIO().DisplaySize;
        const vec4 clip = vp * vec4(world, 1.0f);
        if (clip.w <= 1e-6f) return false; // at/behind the camera
        const vec3 ndc = vec3(clip) / clip.w;
        out.x = (ndc.x * 0.5f + 0.5f) * disp.x;
        out.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * disp.y;
        return true;
    }

    vec2 SketchView::screenToSketch(const mat4& vp, const vec2& screen) const {
        const ImVec2 disp = ImGui::GetIO().DisplaySize;
        const mat4 inv = glm::inverse(vp);
        const float nx = (screen.x / std::max(1.0f, disp.x)) * 2.0f - 1.0f;
        const float ny = 1.0f - (screen.y / std::max(1.0f, disp.y)) * 2.0f;

        // Two clip-space depths along the same pixel give a world-space ray;
        // exact depth convention is irrelevant since we invert the same matrix.
        vec4 n0 = inv * vec4(nx, ny, 0.0f, 1.0f);
        vec4 n1 = inv * vec4(nx, ny, 1.0f, 1.0f);
        const vec3 p0 = vec3(n0) / n0.w;
        const vec3 p1 = vec3(n1) / n1.w;
        const vec3 dir = glm::normalize(p1 - p0);

        const SketchPlane& pl = m_sketch.plane();
        const vec3 N = pl.normal();
        const float denom = glm::dot(dir, N);
        if (std::abs(denom) < 1e-6f) return m_cursorUv; // ray parallel to plane
        const float t = glm::dot(pl.origin() - p0, N) / denom;
        return pl.to2d(p0 + dir * t);
    }

    // ---- picking -----------------------------------------------------------

    PointId SketchView::pickPoint(const mat4& vp, const vec2& mouse, float pixels) const {
        PointId best;
        float bestD = pixels;
        const ImVec2 m{ mouse.x, mouse.y };
        for (const Point& p : m_sketch.points()) {
            if (p.removed) continue;
            vec2 s;
            if (!worldToScreen(vp, m_sketch.plane().to3d(m_sketch.pointPos(p.id)), s)) continue;
            const float d = dist(m, ImVec2{ s.x, s.y });
            if (d < bestD) { bestD = d; best = p.id; }
        }
        return best;
    }

    EntityId SketchView::pickEntity(const mat4& vp, const vec2& mouse, float pixels) const {
        EntityId best;
        float bestD = pixels;
        const ImVec2 m{ mouse.x, mouse.y };
        const SketchPlane& pl = m_sketch.plane();

        for (const Entity& e : m_sketch.entities()) {
            if (e.removed) continue;
            float d = 1e9f;
            if (e.kind == EntityKind::Line) {
                vec2 a, b;
                if (!worldToScreen(vp, pl.to3d(m_sketch.pointPos(e.p0)), a)) continue;
                if (!worldToScreen(vp, pl.to3d(m_sketch.pointPos(e.p1)), b)) continue;
                d = distToSegment(m, { a.x, a.y }, { b.x, b.y });
            }
            else if (e.kind == EntityKind::Circle || e.kind == EntityKind::Arc) {
                const vec2 c = m_sketch.pointPos(e.p0);
                const double r = m_sketch.radius(e.id);
                const int steps = 64;
                const double a0 = e.kind == EntityKind::Arc ? m_sketch.arcStart(e.id) : 0.0;
                double a1 = e.kind == EntityKind::Arc ? m_sketch.arcEnd(e.id) : 2.0 * kPi;
                if (a1 <= a0) a1 += 2.0 * kPi;
                vec2 prevW;
                bool prevOk = false;
                for (int i = 0; i <= steps; ++i) {
                    const double t = a0 + (a1 - a0) * (double(i) / steps);
                    const vec2 uv{ c.x + float(r * std::cos(t)), c.y + float(r * std::sin(t)) };
                    vec2 s;
                    const bool ok = worldToScreen(vp, pl.to3d(uv), s);
                    if (ok && prevOk) {
                        d = std::min(d, distToSegment(m, { prevW.x, prevW.y }, { s.x, s.y }));
                    }
                    prevW = s;
                    prevOk = ok;
                }
            }
            else { // Point entity
                vec2 s;
                if (!worldToScreen(vp, pl.to3d(m_sketch.pointPos(e.p0)), s)) continue;
                d = dist(m, { s.x, s.y });
            }
            if (d < bestD) { bestD = d; best = e.id; }
        }
        return best;
    }

    // ---- editing helpers ---------------------------------------------------

    void SketchView::clearSelection() {
        m_selEntities.clear();
        m_selPoints.clear();
    }

    bool SketchView::isSelected(EntityId e) const {
        return std::find(m_selEntities.begin(), m_selEntities.end(), e) != m_selEntities.end();
    }

    bool SketchView::isSelected(PointId p) const {
        return std::find(m_selPoints.begin(), m_selPoints.end(), p) != m_selPoints.end();
    }

    void SketchView::resolve() {
        m_solve = Solver::solve(m_sketch);
    }

    void SketchView::deleteSelection() {
        for (EntityId e : m_selEntities) m_sketch.removeEntity(e);
        clearSelection();
        resolve();
    }

    void SketchView::applyConstraint(ConstraintKind kind) {
        const auto& sel = m_selEntities;
        switch (kind) {
        case ConstraintKind::Coincident:
            if (m_selPoints.size() == 2) m_sketch.addCoincident(m_selPoints[0], m_selPoints[1]);
            break;
        case ConstraintKind::Horizontal:
        case ConstraintKind::Vertical:
            if (sel.size() == 1) m_sketch.addGeometric(kind, sel[0]);
            break;
        case ConstraintKind::Parallel:
            if (sel.size() == 2) m_sketch.addGeometric(kind, sel[0], sel[1]);
            break;
        case ConstraintKind::Tangent:
        case ConstraintKind::Equal:
            if (sel.size() == 2) m_sketch.addGeometric(kind, sel[0], sel[1]);
            break;
        case ConstraintKind::Distance:
            if (m_selPoints.size() == 2) {
                const float d = glm::length(m_sketch.pointPos(m_selPoints[0]) -
                    m_sketch.pointPos(m_selPoints[1]));
                m_sketch.addDistance(m_selPoints[0], m_selPoints[1], d);
            }
            break;
        case ConstraintKind::Radius:
            if (sel.size() == 1) m_sketch.addRadius(sel[0], m_sketch.radius(sel[0]));
            break;
        case ConstraintKind::Angle:
            if (sel.size() == 2) {
                // default to the current measured angle
                const Entity& e0 = m_sketch.entity(sel[0]);
                const Entity& e1 = m_sketch.entity(sel[1]);
                const vec2 d0 = m_sketch.pointPos(e0.p1) - m_sketch.pointPos(e0.p0);
                const vec2 d1 = m_sketch.pointPos(e1.p1) - m_sketch.pointPos(e1.p0);
                const float ang = std::atan2(d0.x * d1.y - d0.y * d1.x, d0.x * d1.x + d0.y * d1.y);
                m_sketch.addAngle(sel[0], sel[1], ang);
            }
            break;
        }
        resolve();
    }

    // ---- per-frame update --------------------------------------------------

    void SketchView::update(const mat4& vp) {
        if (!m_active) return;

        ImGuiIO& io = ImGui::GetIO();
        const vec2 mouse{ io.MousePos.x, io.MousePos.y };
        const bool overUi = io.WantCaptureMouse;

        m_cursorUv = screenToSketch(vp, mouse);
        m_cursorValid = true;

        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            m_drawing = false;
            m_clickStage = 0;
            clearSelection();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !m_selEntities.empty()) {
            deleteSelection();
        }

        if (!overUi && !m_dragging) {
            // tool-specific click handling
            const bool clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

            if (m_tool == Tool::Select) {
                if (clicked) {
                    const PointId p = pickPoint(vp, mouse, kPickPixels);
                    if (p.valid()) {
                        if (!io.KeyCtrl) clearSelection();
                        if (!isSelected(p)) m_selPoints.push_back(p);
                        if (!m_sketch.point(p).fixed) {
                            m_dragging = true;
                            m_dragPoint = p;
                            m_dragPrevFixed = false;
                            m_sketch.setPointFixed(p, true);
                        }
                    }
                    else {
                        const EntityId e = pickEntity(vp, mouse, kPickPixels);
                        if (!io.KeyCtrl) clearSelection();
                        if (e.valid() && !isSelected(e)) m_selEntities.push_back(e);
                    }
                }
            }
            else if (m_tool == Tool::Line) {
                if (clicked) {
                    PointId p = pickPoint(vp, mouse, kPickPixels);
                    if (!p.valid()) p = m_sketch.addPoint(m_cursorUv.x, m_cursorUv.y);
                    if (!m_drawing) {
                        m_pendStart = p;
                        m_drawing = true;
                    }
                    else if (!(p == m_pendStart)) {
                        m_sketch.addLineFromPoints(m_pendStart, p);
                        m_pendStart = p; // chain
                        resolve();
                    }
                }
            }
            else if (m_tool == Tool::Circle) {
                if (clicked) {
                    if (m_clickStage == 0) {
                        m_pendCenter = m_cursorUv;
                        m_clickStage = 1;
                    }
                    else {
                        const double r = glm::length(m_cursorUv - m_pendCenter);
                        if (r > 1e-4) {
                            m_sketch.addCircle(m_pendCenter.x, m_pendCenter.y, r);
                            resolve();
                        }
                        m_clickStage = 0;
                    }
                }
            }
            else if (m_tool == Tool::Arc) {
                if (clicked) {
                    if (m_clickStage == 0) {
                        m_pendCenter = m_cursorUv;
                        m_clickStage = 1;
                    }
                    else if (m_clickStage == 1) {
                        const vec2 d = m_cursorUv - m_pendCenter;
                        m_pendRadius = glm::length(d);
                        m_pendStartAngle = std::atan2(d.y, d.x);
                        m_clickStage = 2;
                    }
                    else {
                        const vec2 d = m_cursorUv - m_pendCenter;
                        const double endAngle = std::atan2(d.y, d.x);
                        if (m_pendRadius > 1e-4) {
                            m_sketch.addArc(m_pendCenter.x, m_pendCenter.y, m_pendRadius,
                                m_pendStartAngle, endAngle);
                            resolve();
                        }
                        m_clickStage = 0;
                    }
                }
            }
            else if (m_tool == Tool::Point) {
                if (clicked) {
                    m_sketch.addPointEntity(m_cursorUv.x, m_cursorUv.y);
                    resolve();
                }
            }
        }

        // drag in progress
        if (m_dragging) {
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                m_sketch.setPointPos(m_dragPoint, m_cursorUv);
                resolve();
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                m_sketch.setPointFixed(m_dragPoint, m_dragPrevFixed);
                m_dragging = false;
                resolve();
            }
        }

        // ---- overlay ---------------------------------------------------------
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        const SketchPlane& pl = m_sketch.plane();

        // plane axes through the origin (for orientation)
        {
            vec2 o, ax, ay;
            const bool okO = worldToScreen(vp, pl.origin(), o);
            const bool okX = worldToScreen(vp, pl.to3d({ 5.0f, 0.0f }), ax);
            const bool okY = worldToScreen(vp, pl.to3d({ 0.0f, 5.0f }), ay);
            if (okO && okX) dl->AddLine({ o.x, o.y }, { ax.x, ax.y }, kColAxis);
            if (okO && okY) dl->AddLine({ o.x, o.y }, { ay.x, ay.y }, kColAxis);
        }

        auto drawCurve = [&](const Entity& e, ImU32 col, float thick) {
            const vec2 c = m_sketch.pointPos(e.p0);
            const double r = m_sketch.radius(e.id);
            const int steps = 72;
            const double a0 = e.kind == EntityKind::Arc ? m_sketch.arcStart(e.id) : 0.0;
            double a1 = e.kind == EntityKind::Arc ? m_sketch.arcEnd(e.id) : 2.0 * kPi;
            if (a1 <= a0) a1 += 2.0 * kPi;
            for (int i = 0; i < steps; ++i) {
                const double t0 = a0 + (a1 - a0) * (double(i) / steps);
                const double t1 = a0 + (a1 - a0) * (double(i + 1) / steps);
                vec2 s0, s1;
                const vec2 uv0{ c.x + float(r * std::cos(t0)), c.y + float(r * std::sin(t0)) };
                const vec2 uv1{ c.x + float(r * std::cos(t1)), c.y + float(r * std::sin(t1)) };
                if (worldToScreen(vp, pl.to3d(uv0), s0) && worldToScreen(vp, pl.to3d(uv1), s1))
                    dl->AddLine({ s0.x, s0.y }, { s1.x, s1.y }, col, thick);
            }
        };

        for (const Entity& e : m_sketch.entities()) {
            if (e.removed) continue;
            const bool sel = isSelected(e.id);
            const ImU32 col = sel ? kColSel : kColGeom;
            const float th = sel ? 2.5f : 1.5f;
            if (e.kind == EntityKind::Line) {
                vec2 a, b;
                if (worldToScreen(vp, pl.to3d(m_sketch.pointPos(e.p0)), a) &&
                    worldToScreen(vp, pl.to3d(m_sketch.pointPos(e.p1)), b))
                    dl->AddLine({ a.x, a.y }, { b.x, b.y }, col, th);
            }
            else if (e.kind == EntityKind::Circle || e.kind == EntityKind::Arc) {
                drawCurve(e, col, th);
            }
        }

        // points
        for (const Point& p : m_sketch.points()) {
            if (p.removed) continue;
            vec2 s;
            if (!worldToScreen(vp, pl.to3d(m_sketch.pointPos(p.id)), s)) continue;
            const ImU32 col = isSelected(p.id) ? kColSel : (p.fixed ? kColFixed : kColPoint);
            dl->AddRectFilled({ s.x - 3, s.y - 3 }, { s.x + 3, s.y + 3 }, col);
        }

        // dimension labels
        char buf[64];
        for (const Constraint& c : m_sketch.constraints()) {
            if (c.removed || !isDimension(c.kind)) continue;
            vec2 anchor;
            bool ok = false;
            if (c.kind == ConstraintKind::Distance) {
                const vec2 a = m_sketch.pointPos(c.p0), b = m_sketch.pointPos(c.p1);
                ok = worldToScreen(vp, pl.to3d((a + b) * 0.5f), anchor);
                std::snprintf(buf, sizeof(buf), "%.3g", c.value);
            }
            else if (c.kind == ConstraintKind::Radius) {
                ok = worldToScreen(vp, pl.to3d(m_sketch.pointPos(m_sketch.entity(c.e0).p0)), anchor);
                std::snprintf(buf, sizeof(buf), "R%.3g", c.value);
            }
            else { // Angle
                ok = worldToScreen(vp, pl.to3d(m_sketch.pointPos(m_sketch.entity(c.e0).p0)), anchor);
                std::snprintf(buf, sizeof(buf), "%.1f deg", c.value * 180.0 / kPi);
            }
            if (ok) dl->AddText({ anchor.x + 6, anchor.y + 6 }, kColDim, buf);
        }

        // rubber-band previews
        vec2 cs;
        if (worldToScreen(vp, pl.to3d(m_cursorUv), cs)) {
            if (m_tool == Tool::Line && m_drawing) {
                vec2 a;
                if (worldToScreen(vp, pl.to3d(m_sketch.pointPos(m_pendStart)), a))
                    dl->AddLine({ a.x, a.y }, { cs.x, cs.y }, kColPreview, 1.5f);
            }
            else if (m_tool == Tool::Circle && m_clickStage == 1) {
                vec2 c;
                if (worldToScreen(vp, pl.to3d(m_pendCenter), c)) {
                    const float rpx = dist({ c.x, c.y }, { cs.x, cs.y });
                    dl->AddCircle({ c.x, c.y }, rpx, kColPreview, 0, 1.5f);
                }
            }
        }
    }

    // ---- panels ------------------------------------------------------------

    void SketchView::drawPanels() {
        if (!m_active) return;

        if (ImGui::Begin("Sketch")) {
            auto toolButton = [&](const char* label, Tool t) {
                const bool on = m_tool == t;
                if (on) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.5f, 0.1f, 1.0f));
                if (ImGui::Button(label)) {
                    m_tool = t;
                    m_drawing = false;
                    m_clickStage = 0;
                }
                if (on) ImGui::PopStyleColor();
            };

            ImGui::TextUnformatted("Tools");
            toolButton("Select", Tool::Select); ImGui::SameLine();
            toolButton("Line", Tool::Line);     ImGui::SameLine();
            toolButton("Circle", Tool::Circle); ImGui::SameLine();
            toolButton("Arc", Tool::Arc);       ImGui::SameLine();
            toolButton("Point", Tool::Point);

            ImGui::Separator();

            // selection composition, used to enable the right constraint buttons
            int lines = 0, circles = 0;
            for (EntityId e : m_selEntities) {
                const EntityKind k = m_sketch.entity(e).kind;
                if (k == EntityKind::Line) ++lines;
                else if (k == EntityKind::Circle || k == EntityKind::Arc) ++circles;
            }
            const int pts = static_cast<int>(m_selPoints.size());
            const int ents = static_cast<int>(m_selEntities.size());

            auto cbtn = [&](const char* label, ConstraintKind kind, bool enabled) {
                ImGui::BeginDisabled(!enabled);
                if (ImGui::Button(label)) applyConstraint(kind);
                ImGui::EndDisabled();
            };

            ImGui::TextUnformatted("Geometric");
            cbtn("Coincident", ConstraintKind::Coincident, pts == 2); ImGui::SameLine();
            cbtn("Horizontal", ConstraintKind::Horizontal, lines == 1 && ents == 1); ImGui::SameLine();
            cbtn("Vertical", ConstraintKind::Vertical, lines == 1 && ents == 1);
            cbtn("Parallel", ConstraintKind::Parallel, lines == 2); ImGui::SameLine();
            cbtn("Tangent", ConstraintKind::Tangent, ents == 2 && circles >= 1); ImGui::SameLine();
            cbtn("Equal", ConstraintKind::Equal, ents == 2 && (lines == 2 || circles == 2));

            ImGui::TextUnformatted("Dimensional");
            cbtn("Distance", ConstraintKind::Distance, pts == 2); ImGui::SameLine();
            cbtn("Radius", ConstraintKind::Radius, circles == 1 && ents == 1); ImGui::SameLine();
            cbtn("Angle", ConstraintKind::Angle, lines == 2);

            ImGui::Separator();
            ImGui::BeginDisabled(m_selEntities.empty());
            if (ImGui::Button("Delete")) deleteSelection();
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Exit Sketch")) end();

            ImGui::Separator();
            // ---- constraint status (under/over) -------------------------------
            const char* state = m_solve.dof > 0 ? "Under-constrained"
                : (m_solve.overConstrained ? "Over-constrained" : "Fully constrained");
            const ImVec4 col = m_solve.dof > 0 ? ImVec4(1.0f, 0.85f, 0.3f, 1.0f)
                : (m_solve.overConstrained ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f)
                    : ImVec4(0.4f, 1.0f, 0.5f, 1.0f));
            ImGui::TextColored(col, "%s  (DOF %d)", state, m_solve.dof);
            ImGui::Text("vars %d  eqns %d  |r| %.2e%s", m_solve.freeVarCount,
                m_solve.residualCount, m_solve.residualNorm,
                m_solve.converged ? "" : "  [not converged]");
        }
        ImGui::End();

        // ---- constraint list --------------------------------------------------
        if (ImGui::Begin("Constraints")) {
            ConstraintId toDelete;
            bool dirty = false;
            int row = 0;
            for (Constraint& c : m_sketch.constraints()) {
                if (c.removed) continue;
                ImGui::PushID(row++);
                if (ImGui::SmallButton("x")) toDelete = c.id;
                ImGui::SameLine();
                if (isDimension(c.kind)) {
                    ImGui::SetNextItemWidth(90.0f);
                    double v = c.value;
                    const char* fmt = c.kind == ConstraintKind::Angle ? "%.3f rad" : "%.4f";
                    if (ImGui::InputDouble(constraintName(c.kind), &v, 0.0, 0.0, fmt)) {
                        c.value = v;
                        dirty = true;
                    }
                }
                else {
                    ImGui::TextUnformatted(constraintName(c.kind));
                }
                ImGui::PopID();
            }
            if (row == 0) ImGui::TextDisabled("(no constraints)");
            if (toDelete.valid()) { m_sketch.removeConstraint(toDelete); dirty = true; }
            if (dirty) resolve();
        }
        ImGui::End();
    }

} // namespace macad::ui
