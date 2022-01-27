#include "pch.h"
#include "Mesh.h"
#include "RenderSystem.h"

namespace Amadeus
{
	UINT64 Mesh::CreatrPrimitive(Vector<Primitive::Vertex>&& vertices, Vector<UINT>&& indices, INT material, bool normalsProvided, bool tangentsProvided, D3D12_PRIMITIVE_TOPOLOGY mode)
	{
		UINT64 id = mPrimitiveList.size();
		Primitive* primitive = new Primitive(
			std::move(vertices),
			std::move(indices),
			XMLoadFloat4x4(&mModelMatrix),
			material,
			normalsProvided,
			tangentsProvided,
			mode);

		mPrimitiveList.emplace_back(primitive);
		StatBoundary(primitive);
		return id;
	}

	void Mesh::UploadAll(SharedPtr<DeviceResources> device, SharedPtr<RenderSystem> renderer)
	{
		Vector<ID3D12Resource*> uploadHeaps;
		Vector<ID3D12GraphicsCommandList*> commandLists;
		Vector<ID3D12CommandAllocator*> commandAllocators;

		UINT64 size = mPrimitiveList.size();
		for (int i = 0; i < size; ++i)
		{
			ID3D12CommandAllocator* commandAllocator = {};

			ThrowIfFailed(device->GetD3DDevice()->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(&commandAllocator)));

			ID3D12GraphicsCommandList* commandList = {};

			ThrowIfFailed(device->GetD3DDevice()->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				commandAllocator,
				nullptr,
				IID_PPV_ARGS(&commandList)));

			commandAllocators.emplace_back(commandAllocator);
			commandLists.emplace_back(commandList);
		}

		const CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		Vector<Future<bool>> results;
		int i = 0;
		for (auto& primitive : mPrimitiveList)
		{
			const CD3DX12_RESOURCE_DESC verticesDesc = CD3DX12_RESOURCE_DESC::Buffer(primitive->GetVertexDataSize());

			ID3D12Resource* verticesUploadHeap = {};

			ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
				&uploadHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&verticesDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&verticesUploadHeap)));
			uploadHeaps.emplace_back(verticesUploadHeap);

			const CD3DX12_RESOURCE_DESC indicesDesc = CD3DX12_RESOURCE_DESC::Buffer(primitive->GetIndexDataSize());

			ID3D12Resource* indicesUploadHeap = {};

			ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
				&uploadHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&indicesDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&indicesUploadHeap)));
			uploadHeaps.emplace_back(indicesUploadHeap);

#ifdef AMADEUS_CONCURRENCY
			results.emplace_back(renderer->Submit(
				&Primitive::Upload, primitive, device, verticesUploadHeap, indicesUploadHeap, commandLists[i]));
#else
			primitive->Upload(device, verticesUploadHeap, indicesUploadHeap, commandLists[i]);
#endif // AMADEUS_CONCURRENCY

			++i;
		}

		for (auto&& res : results)
		{
			if (!res.get())
				throw RuntimeError("TextureManager::UploadAll Error");
		}

		renderer->Upload(device);

		for (auto&& uploadHeap : uploadHeaps)
		{
			uploadHeap->Release();
		}
		uploadHeaps.clear();

		for (auto&& commandAllocator : commandAllocators)
		{
			commandAllocator->Release();
		}
		commandAllocators.clear();

		for (auto&& commandList : commandLists)
		{
			commandList->Release();
		}
		commandLists.clear();
	}

	void Mesh::RenderShadow(
		SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache, ID3D12GraphicsCommandList* commandList)
	{
		for (auto& primitive : mPrimitiveList)
		{
			primitive->RenderShadow(device, descriptorCache, commandList);
		}
	}

	void Mesh::Render(
		SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache, ID3D12GraphicsCommandList* commandList)
	{
		for (auto& primitive : mPrimitiveList)
		{
			primitive->Render(device, descriptorCache, commandList);
		}
	}

	void Mesh::Destroy()
	{
		for (auto& primitive : mPrimitiveList)
		{
			primitive->Destroy();
		}
		mPrimitiveList.clear();
	}

	void Mesh::StatBoundary(Primitive* primitive)
	{
		const auto& boundary = primitive->GetBoundary();
		mBoundary.xMin = boundary.xMin < mBoundary.xMin ? boundary.xMin : mBoundary.xMin;
		mBoundary.yMin = boundary.yMin < mBoundary.yMin ? boundary.yMin : mBoundary.yMin;
		mBoundary.zMin = boundary.zMin < mBoundary.zMin ? boundary.zMin : mBoundary.zMin;
		mBoundary.xMax = boundary.xMax > mBoundary.xMax ? boundary.xMax : mBoundary.xMax;
		mBoundary.yMax = boundary.yMax > mBoundary.yMax ? boundary.yMax : mBoundary.yMax;
		mBoundary.zMax = boundary.zMax > mBoundary.zMax ? boundary.zMax : mBoundary.zMax;
	}
}