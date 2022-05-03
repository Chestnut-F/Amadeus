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

		XMStoreFloat4x4(&mCameraConstantBuffer.view, XMMatrixIdentity());
		XMStoreFloat4x4(&mCameraConstantBuffer.projection, XMMatrixIdentity());
		XMStoreFloat4x4(&mCameraConstantBuffer.prevViewProjection, XMMatrixIdentity());
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
		XMMATRIX prev = XMMatrixMultiply(XMLoadFloat4x4(&mCameraConstantBuffer.unjitteredProjection), XMLoadFloat4x4(&mCameraConstantBuffer.view));

		XMStoreFloat3(&mPosition, position);
		XMStoreFloat3(&mLookAtPosition, lookAt);

		XMVECTOR forward = lookAt - position;
		XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, forward));
		XMStoreFloat3(&mRightDirection, right);
		XMStoreFloat3(&mUpDirection, XMVector3Normalize(XMVector3Cross(forward, right)));
		XMStoreFloat3(&mLookDirection, XMVector3Normalize(forward));
		XMStoreFloat(&mRadius, XMVector3Length(forward));

		XMStoreFloat4x4(&mCameraConstantBuffer.view, XMMatrixTranspose(GetViewMatrix()));
		XMStoreFloat4x4(&mCameraConstantBuffer.projection, XMMatrixTranspose(GetProjectionMatrix(mSampleIndex)));
		XMStoreFloat4x4(&mCameraConstantBuffer.unjitteredProjection, GetUnjitteredProjectionMatrix());
		mCameraConstantBuffer.eyePosWorld = mPosition;
		mCameraConstantBuffer.nearPlane = mNearPlane;
		mCameraConstantBuffer.farPlane = mFarPlane;
		mCameraConstantBuffer.firstFrame = bFirstFrame;
		XMStoreFloat4x4(&mCameraConstantBuffer.prevViewProjection, prev);

		mSampleIndex = (mSampleIndex + 1) % mNumSamples;
		memcpy(pCameraCbvDataBegin, &mCameraConstantBuffer, mCameraConstantBufferSize);

		bFirstFrame = false;
	}

	void Camera::Destroy()
	{
		mCameraConstants->Release();
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

	XMMATRIX Camera::GetProjectionMatrix(UINT i)
	{
		XMMATRIX res = XMMatrixPerspectiveFovLH(mFov, mAspectRatio, mNearPlane, mFarPlane);
		if (EngineVar::TAA_Enable)
		{
			XMFLOAT2 hammersley = Hammersley2d(i, mNumSamples);
			float jitterX = (hammersley.x * 2.f - 1.f) / (float)SCREEN_WIDTH;
			float jitterY = (hammersley.y * 2.f - 1.f) / (float)SCREEN_HEIGHT;
			mCameraConstantBuffer.jitter = XMFLOAT2(jitterX, -jitterY);
			res.r[2].m128_f32[0] += jitterX;
			res.r[2].m128_f32[1] += jitterY;
		}
		return res;
	}

	XMMATRIX Camera::GetUnjitteredProjectionMatrix()
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

	// Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
	XMFLOAT2 Camera::Hammersley2d(UINT i, UINT N)
	{
		UINT bits = (i << 16u) | (i >> 16u);
		bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
		bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
		bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
		bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
		float rdi = float(bits) * 2.3283064365386963e-10;
		return XMFLOAT2(float(i) / float(N), rdi);
	}
}