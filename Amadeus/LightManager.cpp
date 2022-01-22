#include "pch.h"
#include "LightManager.h"

namespace Amadeus
{
	void LightManager::Init()
	{
		listen<StructureRender>("StructureRender",
			[&](StructureRender params)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = GetDefaultCamera().GetCbvDesc(params.device);
			CD3DX12_GPU_DESCRIPTOR_HANDLE cameraConstantsHandle = params.descriptorCache->AppendCbvCache(params.device, cbvDesc);
			params.commandList->SetGraphicsRootConstantBufferView(COMMON_LIGHT_ROOT_CBV_INDEX, cbvDesc.BufferLocation);
		});
	}
}