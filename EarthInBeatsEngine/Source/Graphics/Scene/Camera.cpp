#include "Camera.h"

void Camera::SetViewport(float width, float height)
{
	m_aspect = width / height;
}

void Camera::OnMouseDrag(float dx, float dy)
{
	m_yaw += dx * 0.005f;
	m_pitch += dy * 0.005f;

	const float limit = DirectX::XM_PIDIV2 - 0.01f;

	if (m_pitch > limit) 
		m_pitch = limit;

	if (m_pitch < -limit) 
		m_pitch = -limit;
}

void Camera::OnMouseWheel(int delta)
{
	m_dist *= (delta > 0 ? 0.9f : 1.1f);

	if (m_dist < 1.0f) 
		m_dist = 1.0f;

	if (m_dist > 200.0f) 
		m_dist = 200.0f;
}

void Camera::SetDistance(float d)
{
	m_dist = d;
}

void Camera::SetTarget(const DirectX::XMFLOAT3& t)
{
	m_target = t;
}

DirectX::XMMATRIX Camera::View() const
{
	const float cy = cosf(m_yaw);
	const float sy = sinf(m_yaw);
	const float cp = cosf(m_pitch);
	const float sp = sinf(m_pitch);

	DirectX::XMFLOAT3 eye {
		m_target.x + m_dist * cp * sy,
		m_target.y + m_dist * sp,
		m_target.z + m_dist * cp * cy
	};

	return DirectX::XMMatrixLookAtRH(XMLoadFloat3(&eye), XMLoadFloat3(&m_target), DirectX::XMVectorSet(0, 1, 0, 0));
}

DirectX::XMMATRIX Camera::Proj() const
{
	return DirectX::XMMatrixPerspectiveFovRH(m_fovY, m_aspect, m_nearZ, m_farZ);
}

DirectX::XMMATRIX Camera::ViewProj() const
{
	return this->View() * this->Proj();
}
