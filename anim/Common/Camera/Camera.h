#pragma once

#include <DirectXMath.h>

namespace anim {
    class Camera {
    public:
        Camera();

        void SetProjectionValues(float fovDegrees, float aspectRatio, float nearZ, float farZ);

        const DirectX::XMMATRIX& GetViewMatrix() const;
        const DirectX::XMMATRIX& GetProjectionMatrix() const;

        const DirectX::XMVECTOR& GetPositionVector() const;
        const DirectX::XMFLOAT3& GetPositionFloat3() const;
        const DirectX::XMVECTOR& GetRotationVector() const;
        const DirectX::XMFLOAT3& GetRotationFloat3() const;

        void SetPosition(const DirectX::XMVECTOR& pos);
        void SetPosition(float x, float y, float z);
        void AdjustPosition(const DirectX::XMVECTOR& pos);
        void AdjustPosition(float x, float y, float z);
        void SetRotation(const DirectX::XMVECTOR& rot);
        void SetRotation(float x, float y, float z);
        void AdjustRotation(const DirectX::XMVECTOR& rot);
        void AdjustRotation(float x, float y, float z);

        void SetLookAtPos(const DirectX::XMFLOAT3& lookAtPos);

        const DirectX::XMVECTOR& GetForwardVector() const;
        const DirectX::XMVECTOR& GetRightVector() const;
        const DirectX::XMVECTOR& GetBackwardVector() const;
        const DirectX::XMVECTOR& GetLeftVector() const;

    private:
        DirectX::XMVECTOR m_posVector;
        DirectX::XMFLOAT3 m_pos;

        DirectX::XMVECTOR m_rotVector;
        DirectX::XMFLOAT3 m_rot;

        DirectX::XMMATRIX m_viewMatrix;
        DirectX::XMMATRIX m_projectionMatrix;

        void UpdateViewMatrix();

        const DirectX::XMVECTOR DEFAULT_FORWARD_VECTOR = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        const DirectX::XMVECTOR DEFAULT_UP_VECTOR = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        const DirectX::XMVECTOR DEFAULT_BACKWARD_VECTOR = DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
        const DirectX::XMVECTOR DEFAULT_LEFT_VECTOR = DirectX::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f);
        const DirectX::XMVECTOR DEFAULT_RIGHT_VECTOR = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

        DirectX::XMVECTOR m_forwardVector;
        DirectX::XMVECTOR m_leftVector;
        DirectX::XMVECTOR m_rightVector;
        DirectX::XMVECTOR m_backwardVector;
    };
}
