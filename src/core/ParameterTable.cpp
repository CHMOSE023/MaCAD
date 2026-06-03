#include "core/ParameterTable.hpp"

#include <stdexcept>

namespace macad {

    void ParameterTable::set(const std::string& name, double value) {
        m_params[name] = value;
    }

    double ParameterTable::get(const std::string& name, double defaultVal) const {
        const auto it = m_params.find(name);
        return it != m_params.end() ? it->second : defaultVal;
    }

    bool ParameterTable::has(const std::string& name) const {
        return m_params.count(name) > 0;
    }

    void ParameterTable::remove(const std::string& name) {
        m_params.erase(name);
    }

    bool ParameterTable::resolve(const std::string& expr, double& outValue) const {
        if (expr.empty()) return false;

        // Try to parse as a literal floating-point number first.
        try {
            std::size_t idx = 0;
            const double v = std::stod(expr, &idx);
            if (idx == expr.size()) {   // entire string consumed → it's a pure literal
                outValue = v;
                return true;
            }
        }
        catch (const std::invalid_argument&) {}
        catch (const std::out_of_range&)     {}

        // Fall back to named parameter lookup.
        const auto it = m_params.find(expr);
        if (it != m_params.end()) {
            outValue = it->second;
            return true;
        }
        return false;
    }

} // namespace macad
