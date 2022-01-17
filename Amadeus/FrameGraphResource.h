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
		FrameGraphResource(
			FrameGraphResourceType type, DXGI_FORMAT format)
			: mType(type)
			, mFormat(format)
			, bRegistered(false)
			, bWritten(false)
			, mFrom(nullptr)
		{

		}

		void RegisterResource(
			SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> cache)
		{
			if (bRegistered)
			{
				bWritten = false;
				return;
			}

			D3D12_RESOURCE_DESC renderTargetDesc;
			renderTargetDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			renderTargetDesc.Alignment = 0;
			renderTargetDesc.Width = device->GetWindowWidth();
			renderTargetDesc.Height = device->GetWindowHeight();
			renderTargetDesc.DepthOrArraySize = 1;
			renderTargetDesc.MipLevels = 1;
			renderTargetDesc.Format = mFormat;
			renderTargetDesc.SampleDesc.Count = 1;
			renderTargetDesc.SampleDesc.Quality = 0;
			renderTargetDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			renderTargetDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

			D3D12_CLEAR_VALUE clearValue = {};
			memcpy(clearValue.Color, BackgroundColor, sizeof(BackgroundColor));
			clearValue.Format = mFormat;

			const CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&renderTargetDesc,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				&clearValue,
				IID_PPV_ARGS(&mResource)
			));
			NAME_D3D12_OBJECT(mResource);

			AppendWriteView(device, cache);

			AppendReadView(device, cache);

			bRegistered = true;
		}

		void UnregisterResource()
		{

		}

		CD3DX12_CPU_DESCRIPTOR_HANDLE GetWriteView(ID3D12GraphicsCommandList* commandList)
		{
			assert(bRegistered);

			const CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
				mResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
			commandList->ResourceBarrier(1, &transition);

			return mWriteHandle;
		}

		CD3DX12_GPU_DESCRIPTOR_HANDLE GetReadView(ID3D12GraphicsCommandList* commandList)
		{
			assert(bWritten);

			const CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
				mResource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			commandList->ResourceBarrier(1, &transition);

			return mReadHandle;
		}

		CD3DX12_CPU_DESCRIPTOR_HANDLE AppendWriteView(
			SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> cache)
		{
			switch (mType)
			{
			case FrameGraphResourceType::RENDER_TARGET:
				return AppendRenderTargetView(device, cache);
			case FrameGraphResourceType::DEPTH:
			case FrameGraphResourceType::STENCIL:
				return AppendDepthStencilView(device, cache);
			default:
				throw Exception("Invaild View Type.");
			}
		}

		CD3DX12_GPU_DESCRIPTOR_HANDLE AppendReadView(
			SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> cache)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = mFormat;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = 1;

			mReadHandle = cache->AppendSrvCache(device, mWriteHandle);
		}

		DependencyGraph::Node* GetFrom()
		{
			return mFrom;
		}

		Vector<DependencyGraph::Node*>& GetTo()
		{
			return mTo;
		}

		void SetFrom(DependencyGraph::Node* from)
		{
			mFrom = from;
		}

		void AddTo(DependencyGraph::Node* to)
		{
			auto iter = std::find(mTo.begin(), mTo.end(), to);
			if (iter == mTo.end())
			{
				mTo.emplace_back(to);
			}
		}

		void Connect(DependencyGraph& graph, DependencyGraph::Node* from, DependencyGraph::Node* to)
		{
			auto iter = std::find(mTo.begin(), mTo.end(), to);
			if (from == mFrom && iter != mTo.end())
			{
				DependencyGraph::Edge(graph, from, to);
			}
		}

	private:
		CD3DX12_CPU_DESCRIPTOR_HANDLE AppendRenderTargetView(
			SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> cache)
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = mFormat;
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Texture2D.MipSlice = 0;
			rtvDesc.Texture2D.PlaneSlice = 0;

			mWriteHandle = cache->AppendRtvCache(device, mResource.Get(), rtvDesc);
			return mWriteHandle;
		}

		CD3DX12_CPU_DESCRIPTOR_HANDLE AppendDepthStencilView(
			SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> cache)
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = mFormat;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

			mWriteHandle = cache->AppendDsvCache(device, mResource.Get(), dsvDesc);
			return mWriteHandle;
		}

		FrameGraphResourceType mType;
		DXGI_FORMAT mFormat;

		ComPtr<ID3D12Resource> mResource;
		CD3DX12_CPU_DESCRIPTOR_HANDLE mWriteHandle;
		CD3DX12_GPU_DESCRIPTOR_HANDLE mReadHandle;

		bool bRegistered;
		bool bWritten;

		DependencyGraph::Node* mFrom;
		Vector<DependencyGraph::Node*> mTo;
	};
}