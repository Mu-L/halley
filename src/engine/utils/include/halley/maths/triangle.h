#pragma once

#include "rect.h"
#include "vector2.h"
#include "vector3.h"

namespace Halley {
    class Polygon;

    class Triangle {
    public:
        Vector2f a, b, c;

        Triangle() = default;
        Triangle(Vector2f a, Vector2f b, Vector2f c)
	        : a(a), b(b), c(c)
        {}

        Vector3f getBarycentricCoordinates(Vector2f p) const;
        Rect4f getBounds() const;
        bool contains(Vector2f p) const;

    	float getArea() const;
        float getPerimeter() const;

        Polygon toPolygon() const;

        Triangle operator+(Vector2f v) const;
        Triangle operator-(Vector2f v) const;
        Triangle operator*(Vector2f v) const;
        Triangle operator/(Vector2f v) const;
        Triangle operator*(float scalar) const;
        Triangle operator/(float scalar) const;

        bool operator==(const Triangle& other) const;
        bool operator!=(const Triangle& other) const;
    };
}
