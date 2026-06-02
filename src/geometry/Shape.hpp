#pragma once

// A thin, OCCT-hiding wrapper around TopoDS_Shape (pimpl). This lets the app
// layer hold and pass shapes around without including any OCCT headers; only
// the geometry .cpp files touch the real OCCT type.

#include <memory>

class TopoDS_Shape; // fwd

namespace macad::geometry {

    class Shape {
    public:
        Shape();
        ~Shape();
        Shape(const Shape& other);
        Shape& operator=(const Shape& other);
        Shape(Shape&&) noexcept;
        Shape& operator=(Shape&&) noexcept;

        // Construct from an OCCT shape (geometry layer internal use).
        explicit Shape(const TopoDS_Shape& shape);

        bool isNull() const;

        // Access the underlying OCCT shape. Defined in Shape.cpp; only callable
        // from translation units that include the OCCT headers.
        const TopoDS_Shape& occt() const;
        TopoDS_Shape& occt();

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace macad::geometry
