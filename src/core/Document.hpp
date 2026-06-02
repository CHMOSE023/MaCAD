#pragma once

// RESERVED (M3/M4/M5). The document is the root model object that will own the
// feature tree, parameters, sketches and assembly structure. Milestone 1 only
// defines the boundary so app/ui can hold a handle without committing to the
// eventual data model.

#include "core/Types.hpp"

namespace macad {

    struct FeatureTag {};
    using FeatureId = StrongId<FeatureTag>;

    // Root of a CAD model. Future responsibilities: feature tree, parameter table,
    // undo/redo stack, serialization. Intentionally minimal for now.
    class IDocument {
    public:
        virtual ~IDocument() = default;

        // Recompute the model after a parameter or feature edit. No-op until M4.
        virtual void recompute() = 0;
    };

} // namespace macad
