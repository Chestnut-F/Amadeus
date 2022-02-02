#pragma once
#include "Prerequisites.h"
#include "DependencyGraph.h"

namespace Amadeus
{
	enum class FrameGraphResourceType
	{
		RENDER_TARGET,
		DEPTH,
		STENCIL,
	};

	class FrameGraphResource
	{
	public:
		FrameGraphResource(const String& name, FrameGraphResourceType type, DXGI_FORMAT format, DependencyGraph::Node* from);

		void RegisterResource(
			SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> cache);

		void RegisterResource(
			SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> cache, UINT64 width, UINT height);

		void UnregisterResource();

		CD3DX12_CPU_DESCRIPTOR_HANDLE GetWriteView(ID3D12GraphicsCommandList* commandList);

		CD3DX12_GPU_DESCRIPTOR_HANDLE GetReadView(ID3D12GraphicsCommandList* commandList);

		CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView(ID3D12GraphicsCommandList* commandList);

		CD3DX12_CPU_DESCRIPTOR_HANDLE AppendWriteView(
			SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> cache);

		CD3DX12_GPU_DESCRIPTOR_HANDLE AppendReadView(
			SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> cache);

		void Connect(DependencyGraph& graph, DependencyGraph::Node* to);

		void Destroy();

	private:
		CD3DX12_CPU_DESCRIPTOR_HANDLE AppendRenderTargetView(
			SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> cache);

		CD3DX12_CPU_DESCRIPTOR_HANDLE AppendDepthStencilView(
			SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> cache);

		void RegisterRenderTarget(SharedPtr<DeviceResources> device);

		void RegisterDepthStencil(SharedPtr<DeviceResources> device);

		void RegisterRenderTarget(SharedPtr<DeviceResources> device, UINT64 width, UINT height);

		void RegisterDepthStencil(SharedPtr<DeviceResources> device, UINT64 width, UINT height);

		String mName;
		FrameGraphResourceType mType;
		DXGI_FORMAT mFormat;

		ComPtr<ID3D12Resource> mResource;
		CD3DX12_CPU_DESCRIPTOR_HANDLE mWriteHandle;
		CD3DX12_GPU_DESCRIPTOR_HANDLE mReadHandle;

		bool bRegistered;
		bool bWritten;
		bool bRead;
		bool bEarlyZ;

		DependencyGraph::Node* mFrom;
		Vector<DependencyGraph::Node*> mTo;
		Vector<DependencyGraph::Edge*> mEdges;
	};
}