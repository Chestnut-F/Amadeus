#include "pch.h"
#include "Common/DescriptorCache.h"

namespace Amadeus
{
	DescriptorCache::DescriptorCache(std::shared_ptr<DeviceResources> device)
		: mRtvCacheOffset(0)
		, mDsvCacheOffset(0)
	{
		for (UINT i = 0; i < c_frameCount; ++i)
			mCbvSrvUavCacheOffsets[i] = 0;

		CreateCbvSrvUavCache(device);
		CreateRtvCache(device);
		CreateDsvCache(device);
	}

	void DescriptorCache::Reset(std::shared_ptr<DeviceResources> device)
	{
		ResetCbvSrvUavCache(device);
		ResetRtvCache();
		ResetDsvCache();
	}

	CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorCache::AppendSrvCache(
		std::shared_ptr<DeviceResources> device, const D3D12_CPU_DESCRIPTOR_HANDLE& srcHandle)
	{
		UINT curFrameIndex = device->GetCurrentFrameIndex();
		assert(mCbvSrvUavCacheOffsets[curFrameIndex] + 1 <= CBV_SRV_UAV_CACHE_SIZE);

		CD3DX12_CPU_DESCRIPTOR_HANDLE dstHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			mCbvSrvUavCaches[curFrameIndex]->GetCPUDescriptorHandleForHeapStart(), 
			mCbvSrvUavCacheOffsets[curFrameIndex], 
			mCbvSrvUavDescriptorSize);

		device->GetD3DDevice()->CopyDescriptors(1, &dstHandle, nullptr, 1, &srcHandle, nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
			mCbvSrvUavCaches[curFrameIndex]->GetGPUDescriptorHandleForHeapStart(), 
			mCbvSrvUavCacheOffsets[curFrameIndex], 
			mCbvSrvUavDescriptorSize);

		++mCbvSrvUavCacheOffsets[curFrameIndex];

		return gpuHandle;
	}

	CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorCache::AppendCbvCache(
		std::shared_ptr<DeviceResources> device, const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc)
	{
		UINT curFrameIndex = device->GetCurrentFrameIndex();
		assert(mCbvSrvUavCacheOffsets[curFrameIndex] + 1 <= CBV_SRV_UAV_CACHE_SIZE);

		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			mCbvSrvUavCaches[curFrameIndex]->GetCPUDescriptorHandleForHeapStart(),
			mCbvSrvUavCacheOffsets[curFrameIndex],
			mCbvSrvUavDescriptorSize);

		device->GetD3DDevice()->CreateConstantBufferView(&cbvDesc, cpuHandle);

		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
			mCbvSrvUavCaches[curFrameIndex]->GetGPUDescriptorHandleForHeapStart(),
			mCbvSrvUavCacheOffsets[curFrameIndex],
			mCbvSrvUavDescriptorSize);

		++mCbvSrvUavCacheOffsets[curFrameIndex];

		return gpuHandle;
	}

	CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorCache::AppendSrvCache(
		std::shared_ptr<DeviceResources> device, ID3D12Resource* renderTarget, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)
	{
		UINT curFrameIndex = device->GetCurrentFrameIndex();
		assert(mCbvSrvUavCacheOffsets[curFrameIndex] + 1 <= CBV_SRV_UAV_CACHE_SIZE);

		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			mCbvSrvUavCaches[curFrameIndex]->GetCPUDescriptorHandleForHeapStart(),
			mCbvSrvUavCacheOffsets[curFrameIndex],
			mCbvSrvUavDescriptorSize);

		device->GetD3DDevice()->CreateShaderResourceView(renderTarget, &srvDesc, cpuHandle);

		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
			mCbvSrvUavCaches[curFrameIndex]->GetGPUDescriptorHandleForHeapStart(),
			mCbvSrvUavCacheOffsets[curFrameIndex],
			mCbvSrvUavDescriptorSize);

		++mCbvSrvUavCacheOffsets[curFrameIndex];

		return gpuHandle;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorCache::AppendRtvCache(
		std::shared_ptr<DeviceResources> device, ID3D12Resource* renderTarget, const D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc)
	{
		assert(mRtvCacheOffset + 1 <= RTV_CACHE_SIZE);

		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			mRtvCache->GetCPUDescriptorHandleForHeapStart(), mRtvCacheOffset, mRtvDescriptorSize);

		device->GetD3DDevice()->CreateRenderTargetView(renderTarget, &rtvDesc, cpuHandle);

		++mRtvCacheOffset;

		return cpuHandle;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorCache::AppendDsvCache(
		std::shared_ptr<DeviceResources> device, ID3D12Resource* depthStencil, const D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc)
	{
		assert(mDsvCacheOffset + 1 <= DSV_CACHE_SIZE);

		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			mDsvCache->GetCPUDescriptorHandleForHeapStart(), mDsvCacheOffset, mDsvDescriptorSize);

		device->GetD3DDevice()->CreateDepthStencilView(depthStencil, &dsvDesc, cpuHandle);

		++mDsvCacheOffset;

		return cpuHandle;
	}

	void DescriptorCache::Destroy()
	{
		for (auto& cache : mCbvSrvUavCaches)
		{
			cache->Release();
		}
		mRtvCache->Release();
		mDsvCache->Release();
	}

	void DescriptorCache::CreateCbvSrvUavCache(std::shared_ptr<DeviceResources> device)
	{
		// Describe and create a cbv srv uav descriptor cache.
		D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavHeapDesc = {};
		cbvSrvUavHeapDesc.NumDescriptors = CBV_SRV_UAV_CACHE_SIZE;
		cbvSrvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvSrvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		for (UINT i = 0; i < c_frameCount; ++i)
		{
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = {};
			ThrowIfFailed(device->GetD3DDevice()->CreateDescriptorHeap(&cbvSrvUavHeapDesc, IID_PPV_ARGS(&descriptorHeap)));
			mCbvSrvUavCaches.emplace_back(descriptorHeap);
			NAME_D3D12_OBJECT_INDEXED(mCbvSrvUavCaches, i);
		}

		mCbvSrvUavDescriptorSize = device->GetD3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	void DescriptorCache::CreateRtvCache(std::shared_ptr<DeviceResources> device)
	{
		// Describe and create a render target view (RTV) descriptor cache.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = RTV_CACHE_SIZE;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->GetD3DDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvCache)));

		mRtvDescriptorSize = device->GetD3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		NAME_D3D12_OBJECT(mRtvCache);
	}

	void DescriptorCache::CreateDsvCache(std::shared_ptr<DeviceResources> device)
	{
		// Describe and create a render target view (DSV) descriptor cache.
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = DSV_CACHE_SIZE;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->GetD3DDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvCache)));

		mDsvDescriptorSize = device->GetD3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		NAME_D3D12_OBJECT(mDsvCache);
	}

	void DescriptorCache::ResetCbvSrvUavCache(std::shared_ptr<DeviceResources> device)
	{
		mCbvSrvUavCacheOffsets[device->GetCurrentFrameIndex()] = 0;
	}

	void DescriptorCache::ResetRtvCache()
	{
		mRtvCacheOffset = 0;
	}

	void DescriptorCache::ResetDsvCache()
	{
		mDsvCacheOffset = 0;
	}


}