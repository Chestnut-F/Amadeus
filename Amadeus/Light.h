#pragma once
#include "Prerequisites.h"

namespace Amadeus
{
	enum class LightType
	{
		SUN,
		POINT,
		SPOT,
	};

	class Light
	{
	public:
		struct LightConstantBuffer
		{
			XMFLOAT4X4 view;
			XMFLOAT4X4 projection;
			XMFLOAT3 position;
			XMFLOAT3 direction;
			XMFLOAT3 color;
			float intensity;
			float nearPlane;
			float farPlane;
			float padding[20];
		};
		static_assert((sizeof(LightConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

	public:
		explicit Light(XMVECTOR pos, XMVECTOR at, XMVECTOR color = { 1.0f, 1.0f, 1.0f }, float intensity = 100000.0f);

		void Upload(SharedPtr<DeviceResources> device);

		void Update();

		void Render();

		void Destroy();

		XMMATRIX GetViewMatrix();

		XMMATRIX GetProjectionMatrix();

		D3D12_CONSTANT_BUFFER_VIEW_DESC GetCbvDesc(SharedPtr<DeviceResources> device);

	private:
		friend class LightManager;

		LightType mType = LightType::SUN;
		XMFLOAT3 mPosition;
		XMFLOAT3 mDirection;
		XMFLOAT3 mColor;
		float mIntensity;
		UINT mIntensityUnit;

		ComPtr<ID3D12Resource> mLightConstants;
		LightConstantBuffer mLightConstantBuffer;
		UINT8* pLightCbvDataBegin;
		const UINT mLightConstantBufferSize = sizeof(LightConstantBuffer);

		bool bCastShadows;

		static const float mWidth;
		static const float mHeight;
		static const float mNearPlane;
		static const float mFarPlane;
	};
}