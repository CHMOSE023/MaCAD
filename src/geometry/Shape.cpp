#include "geometry/Shape.hpp"

#include <TopoDS_Shape.hxx>

namespace macad::geometry {

    struct Shape::Impl {
        TopoDS_Shape shape;
    };

    Shape::Shape() : m_impl(std::make_unique<Impl>()) {}
    Shape::~Shape() = default;

    Shape::Shape(const Shape& other)
        : m_impl(std::make_unique<Impl>(*other.m_impl)) {
    }

    Shape& Shape::operator=(const Shape& other) {
        if (this != &other) {
            m_impl = std::make_unique<Impl>(*other.m_impl);
        }
        return *this;
    }

    Shape::Shape(Shape&&) noexcept = default;
    Shape& Shape::operator=(Shape&&) noexcept = default;

    Shape::Shape(const TopoDS_Shape& shape) : m_impl(std::make_unique<Impl>()) {
        m_impl->shape = shape;
    }

    bool Shape::isNull() const {
        return m_impl->shape.IsNull();
    }

    const TopoDS_Shape& Shape::occt() const { return m_impl->shape; }
    TopoDS_Shape& Shape::occt() { return m_impl->shape; }

} // namespace macad::geometry
