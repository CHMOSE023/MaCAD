#pragma once

// Interactive 2D sketch editor. Owns a sketch::Sketch and drives it entirely
// through ImGui: geometry is drawn as a screen-space overlay (projected with the
// camera's view-projection matrix), and picking/dragging happen in screen space.
// This keeps the UI layer free of bgfx/OCCT -- it only needs glm (via core) and
// the sketch model.
//
// Mouse routing: while active(), the app gives the left button to the sketch
// (draw/drag/select) and leaves right-drag/middle-drag/wheel to the camera so
// the user can still navigate around the sketch plane.

#include "core/Types.hpp"
#include "sketch/Sketch.hpp"
#include "sketch/Solver.hpp"

#include <vector>

namespace macad::ui
{

    class SketchView 
    {
    public:
        enum class Tool { Select, Line, Circle, Arc, Point };

        bool active() const { return m_active; }
        // Starts a fresh sketch on the world XY plane (anchored origin) and
        // enters sketch mode. Ends it if already active.
        void toggle();
        void end();

        // Read-only access to the current sketch (for extrude, etc.).
        const sketch::Sketch& sketch() const { return m_sketch; }

        // Pending extrude request: set when the user clicks "Extrude" in the
        // panel. The application consumes this each frame via takePendingExtrude().
        bool               hasPendingExtrude()    const { return m_pendingExtrude; }
        // The extrude param is a literal ("2.5") or a parameter name ("height").
        const std::string& pendingExtrudeParam()  const { return m_extrudeParam; }
        void               clearPendingExtrude()        { m_pendingExtrude = false; }

        bool               hasPendingRevolve()    const { return m_pendingRevolve; }
        const std::string& pendingRevolveParam()  const { return m_revolveParam; }
        bool               pendingRevolveAroundV() const { return m_revolveAroundV; }
        void               clearPendingRevolve()        { m_pendingRevolve = false; }

        // Per-frame input + overlay rendering. `viewProj` is proj * view from the
        // camera; mouse position and viewport are read from ImGui IO.
        void update(const mat4& viewProj);

        // Docked panels: tool/constraint toolbar and the constraint list.
        void drawPanels();

    private:
        // Projection helpers (screen space matches ImGui MousePos / DisplaySize).
        bool worldToScreen (const mat4& viewProj, const vec3& world, vec2& outScreen) const;
        vec2 screenToSketch(const mat4& viewProj, const vec2& screen) const;

        // Picking.
        sketch::PointId  pickPoint (const mat4& viewProj, const vec2& mouse, float pixels) const;
        sketch::EntityId pickEntity(const mat4& viewProj, const vec2& mouse, float pixels) const;

        // Editing.
        void applyConstraint(sketch::ConstraintKind kind);
        void deleteSelection();
        void resolve();
        void clearSelection();
        bool isSelected(sketch::EntityId e) const;
        bool isSelected(sketch::PointId p) const;

        sketch::Sketch      m_sketch;
        sketch::SolveResult m_solve;

        bool m_active{ false };
        Tool m_tool{ Tool::Select };

        // Selection.
        std::vector<sketch::EntityId> m_selEntities;
        std::vector<sketch::PointId> m_selPoints;

        // In-progress construction (chained for lines).
        bool m_drawing{ false };
        int m_clickStage{ 0 };
        sketch::PointId m_pendStart;       // line start / circle center / arc center
        vec2 m_pendCenter{ 0.0f, 0.0f };
        double m_pendRadius{ 0.0 };
        double m_pendStartAngle{ 0.0 };

        // Dragging a point.
        bool m_dragging{ false };
        sketch::PointId m_dragPoint;
        bool m_dragPrevFixed{ false };

        // Last picked-plane cursor (for rubber-band previews).
        vec2 m_cursorUv{ 0.0f, 0.0f };
        bool m_cursorValid{ false };

        // Extrude request pending consumption by the application.
        bool        m_pendingExtrude{ false };
        std::string m_extrudeParam{ "1.0" };   // literal or parameter name

        // Revolve request pending consumption by the application.
        bool        m_pendingRevolve{ false };
        std::string m_revolveParam{ "360.0" };  // literal or parameter name
        bool        m_revolveAroundV{ true };
    };

}  

