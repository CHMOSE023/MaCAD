#pragma once

// Assembly data types shared between the app and ui layers.
// Pure data structs — no OCCT, no GPU, no rendering.

#include <string>

#include <glm/glm.hpp>

namespace macad {

    // Per-feature transform. Each field is a literal ("1.5") or a parameter
    // name ("height") resolved at solve-time via ParameterTable.
    struct FeatureTransform {
        std::string tx{ "0" }, ty{ "0" }, tz{ "0" };  // translation (world units)
        std::string rx{ "0" }, ry{ "0" }, rz{ "0" };  // rotation, degrees, Euler XYZ
    };

    // Relationship between two features. The solver drives featureB's position
    // to satisfy the constraint relative to featureA.
    enum class AsmConstraintKind {
        ZStack,     // B.tz = A.tz + A_worldHeight          (stack B on top of A)
        XDistance,  // B.tx = A.tx + value
        YDistance,  // B.ty = A.ty + value
        ZDistance,  // B.tz = A.tz + value
        AlignX,     // B.tx = A.tx
        AlignY,     // B.ty = A.ty
        AlignZ,     // B.tz = A.tz
    };

    inline const char* asmConstraintName(AsmConstraintKind k) {
        switch (k) {
        case AsmConstraintKind::ZStack:    return "Z-Stack";
        case AsmConstraintKind::XDistance: return "X-Distance";
        case AsmConstraintKind::YDistance: return "Y-Distance";
        case AsmConstraintKind::ZDistance: return "Z-Distance";
        case AsmConstraintKind::AlignX:    return "Align X";
        case AsmConstraintKind::AlignY:    return "Align Y";
        case AsmConstraintKind::AlignZ:    return "Align Z";
        }
        return "Unknown";
    }

    struct AsmConstraint {
        std::string         name;
        AsmConstraintKind   kind{ AsmConstraintKind::ZStack };
        int                 featureA{ -1 };   // driver
        int                 featureB{ -1 };   // driven
        std::string         value{ "0" };     // for *Distance constraints
    };

    // ========================================================================
    // M5 — full assembly: component instances + mates + iterative solver
    // ========================================================================

    // A placed instance of a part. The geometry comes from a feature (m_features
    // in the app); the same part may be instanced many times, each with its own
    // pose. The solver works directly on the numeric pose fields below.
    struct Component {
        std::string name;
        int         part{ -1 };         // index into features[] supplying geometry
        bool        grounded{ false };  // anchored: the solver never moves it

        // Pose (solver state). Rotation is degrees, Euler XYZ; matches buildMatrix.
        double tx{ 0 }, ty{ 0 }, tz{ 0 };
        double rx{ 0 }, ry{ 0 }, rz{ 0 };

        glm::mat4 world{ 1.0f };        // computed by the app after each solve
    };

    enum class MateAxis { X, Y, Z };

    inline const char* mateAxisName(MateAxis a) {
        switch (a) {
        case MateAxis::X: return "X";
        case MateAxis::Y: return "Y";
        case MateAxis::Z: return "Z";
        }
        return "?";
    }

    inline int mateAxisIndex(MateAxis a) {
        return a == MateAxis::X ? 0 : a == MateAxis::Y ? 1 : 2;
    }

    // A relationship between two components. `b` is the driven side; `a` is the
    // reference. When `b` is grounded but `a` is not, the solver drives `a`.
    enum class MateKind {
        Coincident,  // B.origin == A.origin                 (locks 3 translations)
        Distance,    // B.axis == A.axis + value             (locks 1 translation)
        Concentric,  // share A's axis line + parallel axes  (locks 2 trans + orientation)
        Parallel,    // B.orientation == A.orientation       (orientation only)
        Angle,       // B.orientation == A.orientation + value(deg) about axis
    };

    inline const char* mateKindName(MateKind k) {
        switch (k) {
        case MateKind::Coincident: return "Coincident";
        case MateKind::Distance:   return "Distance";
        case MateKind::Concentric: return "Concentric";
        case MateKind::Parallel:   return "Parallel";
        case MateKind::Angle:      return "Angle";
        }
        return "Unknown";
    }

    inline bool mateNeedsAxis(MateKind k) {
        return k == MateKind::Distance || k == MateKind::Concentric || k == MateKind::Angle;
    }
    inline bool mateNeedsValue(MateKind k) {
        return k == MateKind::Distance || k == MateKind::Angle;
    }

    struct Mate {
        std::string name;
        MateKind    kind{ MateKind::Coincident };
        int         a{ -1 };               // reference component
        int         b{ -1 };               // driven component
        MateAxis    axis{ MateAxis::Z };   // for Distance / Concentric / Angle
        std::string value{ "0" };          // distance or angle(deg); param-resolvable
    };

    // Result of the iterative mate solver, surfaced in the UI.
    struct AssemblyStatus {
        bool        converged{ true };
        int         iterations{ 0 };
        double      residual{ 0.0 };
        int         transDof{ 0 };     // remaining translational DOF estimate
        std::string message;           // human-readable summary
    };

} // namespace macad
