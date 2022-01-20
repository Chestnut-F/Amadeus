#pragma once

namespace Amadeus
{
	static const UINT SRV_HEAP_SIZE = 256;
	static const UINT SAMPLER_HEAP_SIZE = 1;
	static const UINT STANDARD_SAMPLER_ROOT_INDEX = 6;

	class DescriptorManager
	{
	public:
		DescriptorManager(std::shared_ptr<DeviceResources> device);

		CD3DX12_CPU_DESCRIPTOR_HANDLE AllocateSrvHeap(
			std::shared_ptr<DeviceResources> device, ID3D12Resource* texture, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc);

		CD3DX12_CPU_DESCRIPTOR_HANDLE AllocateSamplerHeap(
			std::shared_ptr<DeviceResources> device, const D3D12_SAMPLER_DESC& samplerDesc);

		ID3D12DescriptorHeap* GetSamplerHeap() { return mSamplerHeap.Get(); }

		ID3D12DescriptorHeap* GetSrvHeap() { return mSrvHeap.Get(); }

		void Destroy();

	private:
		void CreateSrvHeap(std::shared_ptr<DeviceResources> device);

		void CreateSamplerHeap(std::shared_ptr<DeviceResources> device);

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvHeap;
		UINT mSrvDescriptorSize;
		UINT mSrvHeapOffset;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSamplerHeap;
		UINT mSamplerDescriptorSize;
		UINT mSamplerHeapOffset;
	};
}