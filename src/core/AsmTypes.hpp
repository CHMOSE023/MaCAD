#pragma once

// Assembly data types shared between the app and ui layers.
// Pure data structs — no OCCT, no GPU, no rendering.

#include <string>

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

} // namespace macad
