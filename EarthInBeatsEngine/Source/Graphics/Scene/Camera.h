#pragma once
#include <DirectXMath.h>

class Camera
{
public:
    Camera() = default;
    ~Camera() = default;

    void SetViewport(float width, float height);

    void OnMouseDrag(float dx, float dy); // pixels
    void OnMouseWheel(int delta);

    void SetDistance(float d);
    void SetTarget(const DirectX::XMFLOAT3& t);

    DirectX::XMMATRIX View() const;
    DirectX::XMMATRIX Proj() const;
    DirectX::XMMATRIX ViewProj() const;

private:
    float m_aspect = 1.0f;
    float m_fovY = DirectX::XM_PIDIV4;   // 45 deg
    float m_nearZ = 0.1f;
    float m_farZ = 1000.0f;

    // Orbit params
    float m_yaw = 0.0f;   // radians
    float m_pitch = 0.0f; // radians
    float m_dist = 5.0f;  // radius
    DirectX::XMFLOAT3 m_target { 0, 0, 0 };
};

