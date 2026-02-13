#pragma once
#include <cmath>
#include <algorithm>

struct Vector2 {
    float x, y;
    Vector2(float x = 0, float y = 0) : x(x), y(y) {}
    Vector2 operator+(const Vector2& o) const { return { x + o.x, y + o.y }; }
    Vector2 operator-(const Vector2& o) const { return { x - o.x, y - o.y }; }
    Vector2 operator*(float s) const { return { x * s, y * s }; }
    float Length() const { return sqrtf(x * x + y * y); }
};

struct Vector3 {
    float x, y, z;
    Vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    Vector3 operator+(const Vector3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vector3 operator-(const Vector3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vector3 operator*(float s) const { return { x * s, y * s, z * s }; }
    float Length() const { return sqrtf(x * x + y * y + z * z); }
    float Length2D() const { return sqrtf(x * x + y * y); }
    float Dot(const Vector3& o) const { return x * o.x + y * o.y + z * o.z; }
    float Distance(const Vector3& o) const { return (*this - o).Length(); }
    float Distance2D(const Vector3& o) const { return (*this - o).Length2D(); }
};

struct Matrix4x4 {
    float m[4][4];
};

namespace Math {
    inline bool WorldToScreen(const Vector3& world, Vector2& screen, const Matrix4x4& viewMatrix, float w, float h) {
        float clipX = viewMatrix.m[0][0] * world.x + viewMatrix.m[0][1] * world.y + viewMatrix.m[0][2] * world.z + viewMatrix.m[0][3];
        float clipY = viewMatrix.m[1][0] * world.x + viewMatrix.m[1][1] * world.y + viewMatrix.m[1][2] * world.z + viewMatrix.m[1][3];
        float clipW = viewMatrix.m[3][0] * world.x + viewMatrix.m[3][1] * world.y + viewMatrix.m[3][2] * world.z + viewMatrix.m[3][3];
        if (clipW < 0.1f) return false;
        float ndcX = clipX / clipW;
        float ndcY = clipY / clipW;
        screen.x = (w / 2.f) * (1.f + ndcX);
        screen.y = (h / 2.f) * (1.f - ndcY);
        return true;
    }

    inline float Clamp(float val, float mn, float mx) {
        return (std::max)(mn, (std::min)(val, mx));
    }

    inline float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }
}
