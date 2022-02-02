#include "pch.h"
#include "FrameGraphResource.h"

namespace Amadeus
{
    FrameGraphResource::FrameGraphResource(
		const String& name, FrameGraphResourceType type, DXGI_FORMAT format, DependencyGraph::Node* from)
		: mName(name)
		, mType(type)
		, mFormat(format)
		, bRegistered(false)
		, bWritten(false)
		, bEarlyZ(false)
		, mFrom(from)
	{

	}

	void FrameGraphResource::RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> cache)
	{
		if (!bRegistered)
		{
			switch (mType)
			{
			case FrameGraphResourceType::RENDER_TARGET:
				RegisterRenderTarget(device);
				break;
			case FrameGraphResourceType::DEPTH:
			case FrameGraphResourceType::STENCIL:
				RegisterDepthStencil(device);
				break;
			default:
				throw Exception("Invaild View Type.");
			}

			bRegistered = true;
		}

		AppendWriteView(device, cache);
		AppendReadView(device, cache);
	}

	void FrameGraphResource::RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> cache, UINT64 width, UINT height)
	{
		if (!bRegistered)
		{
			switch (mType)
			{
			case FrameGraphResourceType::RENDER_TARGET:
				RegisterRenderTarget(device, width, height);
				break;
			case FrameGraphResourceType::DEPTH:
			case FrameGraphResourceType::STENCIL:
				RegisterDepthStencil(device, width, height);
				break;
			default:
				throw Exception("Invaild View Type.");
			}

			bRegistered = true;
		}

		AppendWriteView(device, cache);
		AppendReadView(device, cache);
	}

	void FrameGraphResource::UnregisterResource()
	{

	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE FrameGraphResource::GetWriteView(ID3D12GraphicsCommandList* commandList)
	{
		assert(bRegistered);
		if (bRead)
		{
			CD3DX12_RESOURCE_BARRIER transition;

			switch (mType)
			{
			case FrameGraphResourceType::RENDER_TARGET:
				transition = CD3DX12_RESOURCE_BARRIER::Transition(
					mResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
				commandList->ResourceBarrier(1, &transition);
				break;
			case FrameGraphResourceType::DEPTH:
			case FrameGraphResourceType::STENCIL:
				if (bEarlyZ)
				{
					transition = CD3DX12_RESOURCE_BARRIER::Transition(
						mResource.Get(), D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
					commandList->ResourceBarrier(1, &transition);
				}
				else
				{
					transition = CD3DX12_RESOURCE_BARRIER::Transition(
						mResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
					commandList->ResourceBarrier(1, &transition);
				}
				break;
			default:
				throw Exception("Invaild View Type.");
			}
		}

		bRead = false;
		bEarlyZ = false;

		return mWriteHandle;
	}

	CD3DX12_GPU_DESCRIPTOR_HANDLE FrameGraphResource::GetReadView(ID3D12GraphicsCommandList* commandList)
	{
		assert(bRegistered && !bEarlyZ);
		CD3DX12_RESOURCE_BARRIER transition;

		switch (mType)
		{
		case FrameGraphResourceType::RENDER_TARGET:
			transition = CD3DX12_RESOURCE_BARRIER::Transition(
				mResource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			commandList->ResourceBarrier(1, &transition);
			break;
		case FrameGraphResourceType::DEPTH:
		case FrameGraphResourceType::STENCIL:
			transition = CD3DX12_RESOURCE_BARRIER::Transition(
				mResource.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			commandList->ResourceBarrier(1, &transition);
			break;
		default:
			throw Exception("Invaild View Type.");
		}

		bRead = true;

		return mReadHandle;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE FrameGraphResource::GetDepthStencilView(ID3D12GraphicsCommandList* commandList)
	{
		assert(bRegistered);
		CD3DX12_RESOURCE_BARRIER transition;

		switch (mType)
		{
		case FrameGraphResourceType::DEPTH:
		case FrameGraphResourceType::STENCIL:
			transition = CD3DX12_RESOURCE_BARRIER::Transition(
				mResource.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ);
			commandList->ResourceBarrier(1, &transition);
			break;
		default:
			throw Exception("Invaild View Type.");
		}

		bRead = true;
		bEarlyZ = true;

		return mWriteHandle;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE FrameGraphResource::AppendWriteView(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> cache)
	{
		switch (mType)
		{
		case FrameGraphResourceType::RENDER_TARGET:
			return AppendRenderTargetView(device, cache);
			break;
		case FrameGraphResourceType::DEPTH:
		case FrameGraphResourceType::STENCIL:
			return AppendDepthStencilView(device, cache);
			break;
		default:
			throw Exception("Invaild View Type.");
		}
	}

	CD3DX12_GPU_DESCRIPTOR_HANDLE FrameGraphResource::AppendReadView(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> cache)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		if (mType == FrameGraphResourceType::RENDER_TARGET)
		{
			srvDesc.Format = mFormat;
		}
		else
		{
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;

		mReadHandle = cache->AppendSrvCache(device, mResource.Get(), srvDesc);

		return mReadHandle;
	}

	void FrameGraphResource::Connect(DependencyGraph& graph, DependencyGraph::Node* to)
	{
		auto iter = std::find(mTo.begin(), mTo.end(), to);
		if (iter == mTo.end())
		{
			mTo.emplace_back(to);
			DependencyGraph::Edge* edge = new DependencyGraph::Edge(graph, mFrom, to);
			mEdges.emplace_back(std::move(edge));
		}
	}

	void FrameGraphResource::Destroy()
	{
		if (bRegistered)
			mResource->Release();
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE FrameGraphResource::AppendRenderTargetView(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> cache)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = mFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		mWriteHandle = cache->AppendRtvCache(device, mResource.Get(), rtvDesc);
		return mWriteHandle;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE FrameGraphResource::AppendDepthStencilView(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> cache)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = mFormat;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		mWriteHandle = cache->AppendDsvCache(device, mResource.Get(), dsvDesc);
		return mWriteHandle;
	}

	void FrameGraphResource::RegisterRenderTarget(SharedPtr<DeviceResources> device)
	{
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
		SetName(mResource.Get(), String2WString(mName).c_str());
	}

	void FrameGraphResource::RegisterDepthStencil(SharedPtr<DeviceResources> device)
	{
		D3D12_RESOURCE_DESC depthStencilDesc;
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = device->GetWindowWidth();
		depthStencilDesc.Height = device->GetWindowHeight();
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Format = mFormat;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
		depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil = 0;

		const CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&depthOptimizedClearValue,
			IID_PPV_ARGS(&mResource)
		));
		SetName(mResource.Get(), String2WString(mName).c_str());
	}

	void FrameGraphResource::RegisterRenderTarget(SharedPtr<DeviceResources> device, UINT64 width, UINT height)
	{
		D3D12_RESOURCE_DESC renderTargetDesc;
		renderTargetDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		renderTargetDesc.Alignment = 0;
		renderTargetDesc.Width = width;
		renderTargetDesc.Height = height;
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
		SetName(mResource.Get(), String2WString(mName).c_str());
	}

	void FrameGraphResource::RegisterDepthStencil(SharedPtr<DeviceResources> device, UINT64 width, UINT height)
	{
		D3D12_RESOURCE_DESC depthStencilDesc;
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = width;
		depthStencilDesc.Height = height;
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Format = mFormat;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
		depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil = 0;

		const CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&depthOptimizedClearValue,
			IID_PPV_ARGS(&mResource)
		));
		SetName(mResource.Get(), String2WString(mName).c_str());
	}


}