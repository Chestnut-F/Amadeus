#include "pch.h"
#include "FrameGraphPass.h"

namespace Amadeus
{
	bool FrameGraphPass::PreCompute(SharedPtr<DeviceResources> device, ID3D12GraphicsCommandList* commandList)
	{
		return true;
	}

	void FrameGraphPass::PostPreCompute()
	{
	}

	bool FrameGraphPass::Execute(SharedPtr<DeviceResources> device,
		SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache)
    {
		UINT curFrameIndex = device->GetCurrentFrameIndex();
		auto& commandList = mCommandLists[curFrameIndex];
		ThrowIfFailed(commandList->Reset(device->GetCommandAllocator(), mPipelineState.Get()));
		commandList->SetGraphicsRootSignature(mRootSignature.Get());

		ID3D12DescriptorHeap* ppHeaps[] = { descriptorCache->GetCbvSrvUavCache(device), descriptorManager->GetSamplerHeap() };
		mCommandLists[curFrameIndex]->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		const D3D12_VIEWPORT screenViewPort = device->GetScreenViewport();
		const D3D12_RECT scissorRect = device->GetScissorRect();
		commandList->RSSetViewports(1, &screenViewPort);
		commandList->RSSetScissorRects(1, &scissorRect);

		return true;
    }
}