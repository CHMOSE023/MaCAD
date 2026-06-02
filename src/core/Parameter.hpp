#pragma once

// RESERVED FOR THE PARAMETRIC MILESTONE (M4).
// Only the interface is defined here so other layers can reference parameter
// ids and the evaluation contract. No expression parsing, no dependency graph,
// no recomputation is implemented in Milestone 1.

#include <string>

#include "core/Types.hpp"

namespace macad {

    struct ParameterTag {};
    using ParameterId = StrongId<ParameterTag>;

    // A named, expression-driven scalar parameter. Implementations will own an
    // expression string and a cached evaluated value, and participate in a
    // dependency graph so edits trigger downstream recompute.
    class IParameter {
    public:
        virtual ~IParameter() = default;

        virtual ParameterId id() const = 0;
        virtual const std::string& name() const = 0;

        // The authored expression, e.g. "width / 2 + 1.0".
        virtual const std::string& expression() const = 0;

        // Last evaluated numeric value. Evaluation/dependency tracking is M4.
        virtual double value() const = 0;
    };

} // namespace macad
