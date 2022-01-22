#pragma once
#include "Prerequisites.h"

namespace Amadeus
{
	class Light
	{
	public:
		struct LightConstantBuffer
		{
			float padding[64];
		};
		static_assert((sizeof(LightConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

	public:
		Light();

		void Upload(SharedPtr<DeviceResources> device);

		void Update();

		void Render();

		void Destroy();

		D3D12_CONSTANT_BUFFER_VIEW_DESC GetCbvDesc(SharedPtr<DeviceResources> device);

	private:
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
	};
}