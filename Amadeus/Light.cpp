#include "pch.h"
#include "Light.h"

namespace Amadeus
{
	const float Light::mWidth = 45.0f;
	const float Light::mHeight = 45.0f;
	const float Light::mNearPlane = 0.1f;
	const float Light::mFarPlane = 40.0f;

	Light::Light(XMVECTOR pos, XMVECTOR at, XMVECTOR color, float intensity)
		: mIntensity(intensity)
		, bCastShadows(true)
	{
		XMStoreFloat3(&mColor, color);
		XMStoreFloat3(&mPosition, pos);
		XMStoreFloat3(&mDirection, at - pos);
	}

	void Light::Upload(SharedPtr<DeviceResources> device)
	{
		const CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(mLightConstantBufferSize);

		ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mLightConstants)));

		NAME_D3D12_OBJECT(mLightConstants);

		// Map and initialize the constant buffer. We don't unmap this until the
		// app closes. Keeping things mapped for the lifetime of the resource is okay.
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(mLightConstants->Map(0, &readRange, reinterpret_cast<void**>(&pLightCbvDataBegin)));
	}

	void Light::Update()
	{
		XMStoreFloat4x4(&mLightConstantBuffer.view, XMMatrixTranspose(GetViewMatrix()));
		XMStoreFloat4x4(&mLightConstantBuffer.projection, XMMatrixTranspose(GetProjectionMatrix()));
		XMStoreFloat3(&mLightConstantBuffer.position, XMLoadFloat3(&mPosition));
		XMStoreFloat3(&mLightConstantBuffer.direction, XMLoadFloat3(&mDirection));
		XMStoreFloat3(&mLightConstantBuffer.color, XMLoadFloat3(&mColor));
		mLightConstantBuffer.intensity = mIntensity;
		mLightConstantBuffer.nearPlane = mNearPlane;
		mLightConstantBuffer.farPlane = mFarPlane;
		memcpy(pLightCbvDataBegin, &mLightConstantBuffer, mLightConstantBufferSize);
	}

	void Light::Render()
	{
	}

	void Light::Destroy()
	{
		mLightConstants->Release();
	}

	XMMATRIX Light::GetViewMatrix()
	{
		XMVECTOR forward = XMLoadFloat3(&mDirection);
		XMVECTOR right = XMVector3Normalize(XMVector3Cross({ 0.0f, 1.0f, 0.0f }, forward));
		XMVECTOR up = XMVector3Normalize(XMVector3Cross(forward, right));
		return XMMatrixLookToLH(XMLoadFloat3(&mPosition), forward, up);
	}

	XMMATRIX Light::GetProjectionMatrix()
	{
		return XMMatrixOrthographicLH(mWidth, mHeight, mNearPlane, mFarPlane);
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC Light::GetCbvDesc(SharedPtr<DeviceResources> device)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = mLightConstants->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = mLightConstantBufferSize;
		return std::move(cbvDesc);
	}
}