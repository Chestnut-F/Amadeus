#include "pch.h"
#include "MeshManager.h"
#include "RenderSystem.h"

namespace Amadeus
{
	void MeshManager::Init()
	{
		listen<StructureRender>("StructureRender",
			[&](StructureRender params)
		{
			params.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			MeshManager::Instance().Render(params.device, params.descriptorCache, params.commandList);
		});
	}

	void MeshManager::Destroy()
	{
		for (auto& mesh : mMeshList)
		{
			mesh->Destroy();
		}
	}

	UINT64 MeshManager::CreateMesh(XMMATRIX modelMatrix)
	{
		UINT64 id = mMeshList.size();
		Mesh* mesh = new Mesh(modelMatrix);
		mMeshList.emplace_back(mesh);
		return id;
	}

	void MeshManager::UploadAll(SharedPtr<DeviceResources> device, SharedPtr<RenderSystem> renderer)
	{
		Vector<ID3D12Resource*> uploadHeaps;
		Vector<ID3D12GraphicsCommandList*> commandLists;
		Vector<ID3D12CommandAllocator*> commandAllocators;

		UINT64 size = 0;
		for (auto& mesh : mMeshList)
		{
			size += mesh->GetPrimitiveSize();
		}

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
		for (auto& mesh : mMeshList)
		{
			for (auto& primitive : mesh->GetPrimitives())
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
#endif // DEBUG

				++i;
			}
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
	}

	void MeshManager::Render(
		SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache, ID3D12GraphicsCommandList* commandList)
	{
		for (auto& mesh : mMeshList)
		{
			mesh->Render(device, descriptorCache, commandList);
		}
	}
}