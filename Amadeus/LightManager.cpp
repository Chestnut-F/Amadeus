#include "pch.h"
#include "LightManager.h"

namespace Amadeus
{
	void LightManager::Init()
	{
		listen<ShadowMapRender>("ShadowMapRender",
			[&](ShadowMapRender params)
			{
				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = GetSunLight().GetCbvDesc(params.device);
				CD3DX12_GPU_DESCRIPTOR_HANDLE lightConstantsHandle = params.descriptorCache->AppendCbvCache(params.device, cbvDesc);
				params.commandList->SetGraphicsRootConstantBufferView(COMMON_LIGHT_ROOT_CBV_INDEX, cbvDesc.BufferLocation);
			});

		listen<GBufferRender>("GBufferRender",
			[&](GBufferRender params)
			{
				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = GetSunLight().GetCbvDesc(params.device);
				CD3DX12_GPU_DESCRIPTOR_HANDLE lightConstantsHandle = params.descriptorCache->AppendCbvCache(params.device, cbvDesc);
				params.commandList->SetGraphicsRootConstantBufferView(COMMON_LIGHT_ROOT_CBV_INDEX, cbvDesc.BufferLocation);
			});
	}

	void LightManager::PreRender(float elapsedSeconds)
	{
		auto& light = mLightList[DEFAULT_LIGHT];

		light->Update();
	}

	void LightManager::Render()
	{
	}

	void LightManager::PostRender()
	{
	}

	void LightManager::Destroy()
	{
		for (auto& light : mLightList)
		{
			light->Destroy();
		}
	}

	UINT64 LightManager::Create(XMVECTOR pos, XMVECTOR at, XMVECTOR color, float intensity)
	{
		UINT64 res = mSize;
		Light* camera = new Light(pos, at, color, intensity);
		mLightList.emplace_back(camera);
		mSize++;
		assert(mSize == mLightList.size());

		return res;
	}

	void LightManager::Upload(SharedPtr<DeviceResources> device)
	{
		assert(mSize == 1);

		for (auto& light : mLightList)
		{
			light->Upload(device);
		}
	}
}