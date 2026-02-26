#pragma once
#include <DirectXMath.h>
#include <optional>
#include "../Misc/Types.h"

using namespace DirectX;

inline XMMATRIX ToViewMatrix(const Vector3& rot, const Vector3& origin = Vector3(0, 0, 0)) {
    float radPitch = XMConvertToRadians(rot.x);
    float radYaw = XMConvertToRadians(rot.y);
    float radRoll = XMConvertToRadians(rot.z);

    float sp = std::sin(radPitch), cp = std::cos(radPitch);
    float sy = std::sin(radYaw), cy = std::cos(radYaw);
    float sr = std::sin(radRoll), cr = std::cos(radRoll);

    return XMMATRIX(
        cp * cy, cp * sy, sp, 0.0f,
        sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp, 0.0f,
        -(cr * sp * cy + sr * sy), cy * sr - cr * sp * sy, cr * cp, 0.0f,
        origin.x, origin.y, origin.z, 1.0f
    );
}

// World-to-screen projection.
// Returns nullopt if the point is behind the camera.
inline std::optional<Vector2> WorldToScreen(const Camera& camera, const Vector3& worldPos, int screenWidth, int screenHeight) {
    XMMATRIX viewMatrix = ToViewMatrix(camera.Rotation);

    Vector3 vdelta = worldPos - camera.Location;
    XMVECTOR vdeltaVec = XMVectorSet(vdelta.x, vdelta.y, vdelta.z, 0.0f);

    float transformedX = XMVectorGetX(XMVector3Dot(viewMatrix.r[1], vdeltaVec));
    float transformedY = XMVectorGetX(XMVector3Dot(viewMatrix.r[2], vdeltaVec));
    float transformedZ = XMVectorGetX(XMVector3Dot(viewMatrix.r[0], vdeltaVec));

    // Behind camera
    if (transformedZ < 1.0f)
        return std::nullopt;

    float fov = camera.FieldOfView;
    if (fov <= 0.0f || fov >= 180.0f)
        return std::nullopt;

    float centerX = screenWidth / 2.0f;
    float centerY = screenHeight / 2.0f;
    float tanFov = tanf(fov * static_cast<float>(M_PI) / 360.0f);

    float screenX = centerX + (transformedX / tanFov) * centerX / transformedZ;
    float screenY = centerY - (transformedY / tanFov) * centerX / transformedZ;

    return Vector2(screenX, screenY);
}

// Transform bone position from bone-space to world-space
inline Vector3 BoneToWorld(const FTransform& bone, const FTransform& componentToWorld) {
    XMMATRIX boneMatrix = bone.ToMatrixWithScale();
    XMMATRIX componentMatrix = componentToWorld.ToMatrixWithScale();
    XMMATRIX result = XMMatrixMultiply(boneMatrix, componentMatrix);

    return Vector3(
        XMVectorGetX(result.r[3]),
        XMVectorGetY(result.r[3]),
        XMVectorGetZ(result.r[3])
    );
}
