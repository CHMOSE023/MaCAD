#pragma once

// Foundational type aliases and small utilities shared across all layers.
// Kept dependency-light: only glm and the standard library.

#include <cstdint>
#include <string>
#include <variant>

#include <glm/glm.hpp>

namespace macad {

    using glm::vec2;
    using glm::vec3;
    using glm::vec4;
    using glm::mat3;
    using glm::mat4;

    // Strongly typed id wrapper to avoid mixing different id spaces (e.g. a
    // FeatureId passed where a ParameterId is expected). Tag is a phantom type.
    template <typename Tag>
    struct StrongId {
        std::uint64_t value{ 0 };

        constexpr StrongId() = default;
        constexpr explicit StrongId(std::uint64_t v) : value(v) {}

        constexpr bool valid() const { return value != 0; }
        constexpr bool operator==(const StrongId&) const = default;
    };

    // Minimal Result type for fallible operations without exceptions at API
    // boundaries. E defaults to std::string for human-readable errors.
    template <typename T, typename E = std::string>
    class Result {
    public:
        Result(T value) : m_data(std::move(value)) {}
        static Result error(E e) { return Result(std::move(e), true); }

        bool ok() const { return std::holds_alternative<T>(m_data); }
        explicit operator bool() const { return ok(); }

        T& value() { return std::get<T>(m_data); }
        const T& value() const { return std::get<T>(m_data); }
        const E& error() const { return std::get<E>(m_data); }

    private:
        Result(E e, bool) : m_data(std::move(e)) {}
        std::variant<T, E> m_data;
    };

} // namespace macad
