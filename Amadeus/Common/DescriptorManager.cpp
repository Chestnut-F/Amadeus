#include "pch.h"
#include "Common/DescriptorManager.h"

namespace Amadeus
{
	DescriptorManager::DescriptorManager(std::shared_ptr<DeviceResources> device)
	{
		CreateSrvHeap(device);
		CreateSamplerHeap(device);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorManager::AllocateSrvHeap(
		std::shared_ptr<DeviceResources> device, ID3D12Resource* texture, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)
	{
		assert(mSrvHeapOffset + 1 <= SRV_HEAP_SIZE);

		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			mSrvHeap->GetCPUDescriptorHandleForHeapStart(), mSrvHeapOffset, mSrvDescriptorSize);

		device->GetD3DDevice()->CreateShaderResourceView(texture, &srvDesc, srvHandle);

		++mSrvHeapOffset;

		return srvHandle;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorManager::AllocateSamplerHeap(
		std::shared_ptr<DeviceResources> device, const D3D12_SAMPLER_DESC& samplerDesc)
	{
		assert(mSamplerHeapOffset + 1 <= SAMPLER_HEAP_SIZE);

		CD3DX12_CPU_DESCRIPTOR_HANDLE samplerHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			mSamplerHeap->GetCPUDescriptorHandleForHeapStart(), mSamplerHeapOffset, mSamplerDescriptorSize);

		device->GetD3DDevice()->CreateSampler(&samplerDesc, samplerHandle);

		++mSamplerHeapOffset;

		return samplerHandle;
	}

	void DescriptorManager::CreateSrvHeap(std::shared_ptr<DeviceResources> device)
	{
		D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavHeapDesc = {};
		cbvSrvUavHeapDesc.NumDescriptors = SRV_HEAP_SIZE;
		cbvSrvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvSrvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->GetD3DDevice()->CreateDescriptorHeap(&cbvSrvUavHeapDesc, IID_PPV_ARGS(&mSrvHeap)));

		mSrvDescriptorSize = device->GetD3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		NAME_D3D12_OBJECT(mSrvHeap);
	}

	void DescriptorManager::CreateSamplerHeap(std::shared_ptr<DeviceResources> device)
	{
		D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
		samplerHeapDesc.NumDescriptors = SAMPLER_HEAP_SIZE;
		samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(device->GetD3DDevice()->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&mSamplerHeap)));

		mSamplerDescriptorSize = device->GetD3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

		NAME_D3D12_OBJECT(mSamplerHeap);
	}
}