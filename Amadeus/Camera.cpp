#include "pch.h"
#include "Camera.h"

namespace Amadeus
{
	Camera::Camera(XMVECTOR position, XMVECTOR lookAt, XMVECTOR up,
		float fov, float aspectRatio, float nearPlane, float farPlane)
		: mFov(fov)
		, mAspectRatio(aspectRatio)
		, mNearPlane(nearPlane)
		, mFarPlane(farPlane)
	{
		XMStoreFloat3(&mPosition, position);
		XMStoreFloat3(&mLookAtPosition, lookAt);

		XMVECTOR forward = lookAt - position;
		XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, forward));
		XMStoreFloat3(&mRightDirection, right);
		XMStoreFloat3(&mUpDirection, XMVector3Normalize(XMVector3Cross(forward, right)));
		XMStoreFloat3(&mLookDirection, XMVector3Normalize(forward));
		XMStoreFloat(&mRadius, XMVector3Length(forward));
	}

	void Camera::Upload(SharedPtr<DeviceResources> device)
	{
		const CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(mCameraConstantBufferSize);

		ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mCameraConstants)));

		NAME_D3D12_OBJECT(mCameraConstants);

		// Map and initialize the constant buffer. We don't unmap this until the
		// app closes. Keeping things mapped for the lifetime of the resource is okay.
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(mCameraConstants->Map(0, &readRange, reinterpret_cast<void**>(&pCameraCbvDataBegin)));
	}

	void Camera::Update(XMVECTOR position, XMVECTOR lookAt, XMVECTOR up)
	{
		XMStoreFloat3(&mPosition, position);
		XMStoreFloat3(&mLookAtPosition, lookAt);

		XMVECTOR forward = lookAt - position;
		XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, forward));
		XMStoreFloat3(&mRightDirection, right);
		XMStoreFloat3(&mUpDirection, XMVector3Normalize(XMVector3Cross(forward, right)));
		XMStoreFloat3(&mLookDirection, XMVector3Normalize(forward));
		XMStoreFloat(&mRadius, XMVector3Length(forward));

		XMStoreFloat4x4(&mCameraConstantBuffer.view, XMMatrixTranspose(GetViewMatrix()));
		XMStoreFloat4x4(&mCameraConstantBuffer.projection, XMMatrixTranspose(GetProjectionMatrix()));
		mCameraConstantBuffer.eyePosWorld = mPosition;
		mCameraConstantBuffer.nearPlane = mNearPlane;
		mCameraConstantBuffer.farPlane = mFarPlane;
		memcpy(pCameraCbvDataBegin, &mCameraConstantBuffer, mCameraConstantBufferSize);
	}

	XMVECTOR Camera::GetPosition()
	{
		return XMLoadFloat3(&mPosition);
	}

	XMVECTOR Camera::GetLookAtPosition()
	{
		return XMLoadFloat3(&mLookAtPosition);
	}

	XMVECTOR Camera::GetLookDirection()
	{
		return XMLoadFloat3(&mLookDirection);
	}

	XMVECTOR Camera::GetUpirection()
	{
		return XMLoadFloat3(&mUpDirection);
	}

	float Camera::GetRadius()
	{
		return mRadius;
	}

	XMMATRIX Camera::GetViewMatrix()
	{
		return XMMatrixLookToLH(XMLoadFloat3(&mPosition), XMLoadFloat3(&mLookDirection), XMLoadFloat3(&mUpDirection));
	}

	XMMATRIX Camera::GetProjectionMatrix()
	{
		return XMMatrixPerspectiveFovLH(mFov, mAspectRatio, mNearPlane, mFarPlane);
	}

	XMMATRIX Camera::GetTransformMatrix()
	{
		XMVECTOR scale = { 1.0f, 1.0f, 1.0f };
		XMMATRIX m = { XMLoadFloat3(&mRightDirection), XMLoadFloat3(&mUpDirection), XMLoadFloat3(&mLookDirection), g_XMIdentityR3 };
		XMVECTOR rot = XMQuaternionRotationMatrix(m);
		XMVECTOR center = XMLoadFloat3(&mPosition);
		XMMATRIX res = XMMatrixAffineTransformation(scale, g_XMZero, rot, center);
		return res;
	}

	XMMATRIX Camera::GetTransformMatrixWithoutTranslation()
	{
		XMVECTOR scale = { 1.0f, 1.0f, 1.0f };
		XMMATRIX m = { XMLoadFloat3(&mRightDirection), XMLoadFloat3(&mUpDirection), XMLoadFloat3(&mLookDirection), g_XMIdentityR3 };
		XMVECTOR rot = XMQuaternionRotationMatrix(m);
		XMMATRIX res = XMMatrixAffineTransformation(scale, g_XMZero, rot, g_XMZero);
		return res;
	}

	float Camera::GetNearPlane()
	{
		return mNearPlane;
	}

	float Camera::GetFarPlane()
	{
		return mFarPlane;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC Camera::GetCbvDesc(SharedPtr<DeviceResources> device)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = mCameraConstants->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = mCameraConstantBufferSize;
		return std::move(cbvDesc);
	}
}