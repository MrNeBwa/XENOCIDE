#pragma once
#include <cmath>

namespace mine {

struct Vector2 {
    float x, y;

    Vector2() : x(0.0f), y(0.0f) {}
    Vector2(float x, float y) : x(x), y(y) {}

    Vector2 operator+(const Vector2& other) const {
        return {x + other.x, y + other.y};
    }

    Vector2 operator-(const Vector2& other) const {
        return {x - other.x, y - other.y};
    }

    Vector2 operator*(float scalar) const {
        return {x * scalar, y * scalar};
    }

    Vector2& operator+=(const Vector2& other) {
        x += other.x; y += other.y; return *this;
    }

    Vector2& operator-=(const Vector2& other) {
        x -= other.x; y -= other.y; return *this;
    }

    float length() const {
        return std::sqrt(x * x + y * y);
    }

    float lengthSq() const {
        return x * x + y * y;
    }

    Vector2 normalized() const {
        float len = length();
        if (len < 0.0001f) return {0.0f, 0.0f};
        return {x / len, y / len};
    }

    static float dot(const Vector2& a, const Vector2& b) {
        return a.x * b.x + a.y * b.y;
    }

    static float distance(const Vector2& a, const Vector2& b) {
        return (a - b).length();
    }

    static float distanceSq(const Vector2& a, const Vector2& b) {
        return (a - b).lengthSq();
    }
};

} // namespace mine
