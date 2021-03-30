#include "pch.h"
#include "Camera.h"

using namespace anim;
using namespace DirectX;

Camera::Camera()
{
    m_pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_posVector = XMLoadFloat3(&m_pos);
    m_rot = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_rotVector = XMLoadFloat3(&m_rot);
    UpdateViewMatrix();
}

void Camera::SetProjectionValues(float fovDegrees, float aspectRatio, float nearZ, float farZ)
{
    float fovRadians = (fovDegrees / 360.0f) * XM_2PI;
    m_projectionMatrix = XMMatrixPerspectiveFovRH(fovRadians, aspectRatio, nearZ, farZ);
}

const XMMATRIX& Camera::GetViewMatrix() const
{
    return m_viewMatrix;
}

const XMMATRIX& Camera::GetProjectionMatrix() const
{
    return m_projectionMatrix;
}

const XMVECTOR& Camera::GetPositionVector() const
{
    return m_posVector;
}

const XMFLOAT3& Camera::GetPositionFloat3() const
{
    return m_pos;
}

const XMVECTOR& Camera::GetRotationVector() const
{
    return m_rotVector;
}

const XMFLOAT3& Camera::GetRotationFloat3() const
{
    return m_rot;
}

void Camera::SetPosition(const DirectX::XMVECTOR& pos)
{
    XMStoreFloat3(&m_pos, pos);
    m_posVector = pos;
    UpdateViewMatrix();
}

void Camera::SetPosition(float x, float y, float z)
{
    m_pos = XMFLOAT3(x, y, z);
    m_posVector = XMLoadFloat3(&m_pos);
    UpdateViewMatrix();
}

void Camera::AdjustPosition(const DirectX::XMVECTOR& pos)
{
    m_posVector += pos;
    XMStoreFloat3(&m_pos, m_posVector);
    UpdateViewMatrix();
}

void Camera::AdjustPosition(float x, float y, float z)
{
    m_pos.x += x;
    m_pos.y += y;
    m_pos.z += z;
    m_posVector = XMLoadFloat3(&m_pos);
    UpdateViewMatrix();
}

void Camera::SetRotation(const DirectX::XMVECTOR& rot)
{
    XMStoreFloat3(&m_rot, rot);
    m_rotVector = rot;
    UpdateViewMatrix();
}

void Camera::SetRotation(float x, float y, float z)
{
    m_rot = XMFLOAT3(x, y, z);
    m_rotVector = XMLoadFloat3(&m_rot);
    UpdateViewMatrix();
}

void Camera::AdjustRotation(const DirectX::XMVECTOR& rot)
{
    m_rotVector += rot;
    XMStoreFloat3(&m_rot, m_rotVector);
    UpdateViewMatrix();
}

void Camera::AdjustRotation(float x, float y, float z)
{
    m_rot.x += x;
    m_rot.y += y;
    m_rot.z += z;
    m_rotVector = XMLoadFloat3(&m_rot);
    UpdateViewMatrix();
}

void Camera::SetLookAtPos(const DirectX::XMFLOAT3& lookAtPos)
{
    //Verify that look at pos is not the same as cam pos. They cannot be the same as that wouldn't make sense and would result in undefined behavior.
    if (lookAtPos.x == m_pos.x && lookAtPos.y == m_pos.y && lookAtPos.z == m_pos.z)
        return;

    DirectX::XMFLOAT3 lookAtPosCopy = lookAtPos;
    lookAtPosCopy.x = m_pos.x - lookAtPos.x;
    lookAtPosCopy.y = m_pos.y - lookAtPos.y;
    lookAtPosCopy.z = m_pos.z - lookAtPos.z;

    const float distance = sqrt(lookAtPosCopy.x * lookAtPosCopy.x + lookAtPosCopy.z * lookAtPosCopy.z);
    float pitch = atan2(lookAtPosCopy.y, distance);

    float yaw = -atan2(lookAtPosCopy.x, -lookAtPosCopy.z);

    SetRotation(pitch, yaw, 0.0f);
}

const XMVECTOR& Camera::GetForwardVector() const
{
    return m_forwardVector;
}

const XMVECTOR& Camera::GetRightVector() const
{
    return m_rightVector;
}

const XMVECTOR& Camera::GetBackwardVector() const
{
    return m_backwardVector;
}

const XMVECTOR& Camera::GetLeftVector() const
{
    return m_leftVector;
}

void Camera::UpdateViewMatrix()
{
    // Calculate camera rotation matrix
    XMMATRIX camRotationMatrix = XMMatrixRotationRollPitchYaw(m_rot.x, m_rot.y, m_rot.z);

    // Calculate unit vector of cam target based off camera forward value transformed by cam rotation matrix
    XMVECTOR camTarget = XMVector3TransformCoord(DEFAULT_FORWARD_VECTOR, camRotationMatrix);

    // Adjust cam target to be offset by the camera's current position
    camTarget += m_posVector;

    // Calculate up direction based on current rotation
    XMVECTOR upDir = XMVector3TransformCoord(DEFAULT_UP_VECTOR, camRotationMatrix);

    // Rebuild view matrix
    m_viewMatrix = XMMatrixLookAtRH(m_posVector, camTarget, upDir);

    m_forwardVector = XMVector3TransformCoord(this->DEFAULT_FORWARD_VECTOR, camRotationMatrix);
    m_forwardVector = XMVector3Normalize(m_forwardVector);
    m_backwardVector = XMVector3TransformCoord(this->DEFAULT_BACKWARD_VECTOR, camRotationMatrix);
    m_backwardVector = XMVector3Normalize(m_backwardVector);
    m_leftVector = XMVector3TransformCoord(this->DEFAULT_LEFT_VECTOR, camRotationMatrix);
    m_leftVector = XMVector3Normalize(m_leftVector);
    m_rightVector = XMVector3TransformCoord(this->DEFAULT_RIGHT_VECTOR, camRotationMatrix);
    m_rightVector = XMVector3Normalize(m_rightVector);
}
