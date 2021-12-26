#pragma once

namespace Amadeus
{
	static const UINT CBV_SRV_UAV_CACHE_SIZE = 1024;
	static const UINT RTV_CACHE_SIZE = 1024;
	static const UINT DSV_CACHE_SIZE = 32;

	class DescriptorCache
	{
	public:
		DescriptorCache(std::shared_ptr<DeviceResources> device);

		void Reset();

		CD3DX12_GPU_DESCRIPTOR_HANDLE AppendSrvCache(
			std::shared_ptr<DeviceResources> device, const D3D12_CPU_DESCRIPTOR_HANDLE& srcHandle);

		CD3DX12_GPU_DESCRIPTOR_HANDLE AppendCbvCache(
			std::shared_ptr<DeviceResources> device, const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc);

		CD3DX12_CPU_DESCRIPTOR_HANDLE AppendRtvCache(
			std::shared_ptr<DeviceResources> device, ID3D12Resource* renderTarget, const D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc);

		CD3DX12_CPU_DESCRIPTOR_HANDLE AppendDsvCache(
			std::shared_ptr<DeviceResources> device, ID3D12Resource* depthStencil, const D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc);

		ID3D12DescriptorHeap* GetCbvSrvUavCache() { return mCbvSrvUavCache.Get(); }

	private:
		void CreateCbvSrvUavCache(std::shared_ptr<DeviceResources> device);

		void CreateRtvCache(std::shared_ptr<DeviceResources> device);

		void CreateDsvCache(std::shared_ptr<DeviceResources> device);

		void ResetCbvSrvUavCache();

		void ResetRtvCache();

		void ResetDsvCache();

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvSrvUavCache;
		UINT mCbvSrvUavDescriptorSize;
		UINT mCbvSrvUavCacheOffset;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvCache;
		UINT mRtvDescriptorSize;
		UINT mRtvCacheOffset;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvCache;
		UINT mDsvDescriptorSize;
		UINT mDsvCacheOffset;
	};
}