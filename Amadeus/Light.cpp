#include "pch.h"
#include "Light.h"

namespace Amadeus
{
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
}