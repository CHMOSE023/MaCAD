#pragma once

// Named scalar parameter store with expression resolution.
//
// Resolution rules (ParameterTable::resolve):
//   "3.14"    → literal double
//   "height"  → looks up the named parameter
//   Anything else, or a missing name → returns false
//
// This intentionally keeps expressions trivial for M4. A full expression
// parser (arithmetic, references to other params) belongs in M5+.

#include <map>
#include <string>

namespace macad {

    class ParameterTable {
    public:
        // Create / overwrite a named parameter.
        void   set(const std::string& name, double value);

        // Returns value, or defaultVal if the name is not found.
        double get(const std::string& name, double defaultVal = 0.0) const;

        bool   has(const std::string& name) const;
        void   remove(const std::string& name);

        // Read-only view of all parameters (for UI iteration).
        const std::map<std::string, double>& all() const { return m_params; }

        // Resolve `expr` as either a literal number or a named parameter.
        // Returns true and sets outValue on success; returns false otherwise.
        bool resolve(const std::string& expr, double& outValue) const;

    private:
        std::map<std::string, double> m_params;
    };

} // namespace macad
