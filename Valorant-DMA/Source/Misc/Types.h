#pragma once
#include <cmath>
#include <DirectXMath.h>
#include <xmmintrin.h>

using namespace DirectX;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Vector2 {
public:
    float x, y;

    Vector2() : x(0.f), y(0.f) {}
    Vector2(float x, float y) : x(x), y(y) {}

    Vector2 operator+(const Vector2& v) const { return { x + v.x, y + v.y }; }
    Vector2 operator-(const Vector2& v) const { return { x - v.x, y - v.y }; }
    Vector2 operator*(float s) const { return { x * s, y * s }; }
    Vector2 operator/(float s) const { return { x / s, y / s }; }
    Vector2 operator-() const { return { -x, -y }; }
    bool operator==(const Vector2& v) const { return x == v.x && y == v.y; }
    bool operator!=(const Vector2& v) const { return !(*this == v); }

    float Length() const { return std::sqrt(x * x + y * y); }
    float Distance(const Vector2& v) const { return (*this - v).Length(); }
};

class Vector3 {
public:
    float x, y, z;

    Vector3() : x(0.f), y(0.f), z(0.f) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& v) const { return { x + v.x, y + v.y, z + v.z }; }
    Vector3 operator-(const Vector3& v) const { return { x - v.x, y - v.y, z - v.z }; }
    Vector3 operator*(float s) const { return { x * s, y * s, z * s }; }
    Vector3 operator/(float s) const { return { x / s, y / s, z / s }; }
    Vector3 operator-() const { return { -x, -y, -z }; }
    bool operator==(const Vector3& v) const { return x == v.x && y == v.y && z == v.z; }
    bool operator!=(const Vector3& v) const { return !(*this == v); }

    float Dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
    float Length() const { return std::sqrt(x * x + y * y + z * z); }

    float Distance(const Vector3& v) const {
        float dx = v.x - x, dy = v.y - y, dz = v.z - z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    Vector3 Normalized() const {
        float len = Length();
        if (len < 0.0001f) return { 0, 0, 0 };
        return *this / len;
    }
};

struct FQuat {
    double x, y, z, w;
};

struct alignas(16) FTransform {
    XMVECTOR rot;
    XMVECTOR translation;
    XMVECTOR scale;

    XMMATRIX ToMatrixWithScale() const {
        float rx = XMVectorGetX(rot), ry = XMVectorGetY(rot);
        float rz = XMVectorGetZ(rot), rw = XMVectorGetW(rot);
        float sx = XMVectorGetX(scale), sy = XMVectorGetY(scale), sz = XMVectorGetZ(scale);
        float tx = XMVectorGetX(translation), ty = XMVectorGetY(translation), tz = XMVectorGetZ(translation);

        float x2 = rx + rx, y2 = ry + ry, z2 = rz + rz;
        float xx2 = rx * x2, yy2 = ry * y2, zz2 = rz * z2;
        float yz2 = ry * z2, wx2 = rw * x2, xy2 = rx * y2;
        float wz2 = rw * z2, xz2 = rx * z2, wy2 = rw * y2;

        XMMATRIX m;
        m.r[0] = XMVectorSet((1.0f - (yy2 + zz2)) * sx, (xy2 + wz2) * sx, (xz2 - wy2) * sx, 0.0f);
        m.r[1] = XMVectorSet((xy2 - wz2) * sy, (1.0f - (xx2 + zz2)) * sy, (yz2 + wx2) * sy, 0.0f);
        m.r[2] = XMVectorSet((xz2 + wy2) * sz, (yz2 - wx2) * sz, (1.0f - (xx2 + yy2)) * sz, 0.0f);
        m.r[3] = XMVectorSet(tx, ty, tz, 1.0f);
        return m;
    }

    bool IsValid() const {
        float rx = XMVectorGetX(rot), ry = XMVectorGetY(rot);
        float rz = XMVectorGetZ(rot), rw = XMVectorGetW(rot);
        float lenSq = rx * rx + ry * ry + rz * rz + rw * rw;
        if (std::fabsf(lenSq - 1.0f) > 0.001f) return false;

        float sx = XMVectorGetX(scale), sy = XMVectorGetY(scale), sz = XMVectorGetZ(scale);
        if (sx <= 0.0f || sy <= 0.0f || sz <= 0.0f) return false;
        if (std::isnan(sx) || std::isnan(sy) || std::isnan(sz)) return false;

        float tx = XMVectorGetX(translation), ty = XMVectorGetY(translation), tz = XMVectorGetZ(translation);
        if (std::isnan(tx) || std::isnan(ty) || std::isnan(tz)) return false;

        return true;
    }
};

struct Camera {
    Vector3 Location;
    Vector3 Rotation;
    float FieldOfView;
};

typedef struct MMatrix {
    union {
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };
} MMatrix;
