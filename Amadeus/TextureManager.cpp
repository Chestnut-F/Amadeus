#include "pch.h"
#include "TextureManager.h"

namespace Amadeus
{
	UINT64 TextureManager::LoadFromFile(
		WString&& fileName, SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager)
	{
		UINT64 id = mTextureIndices.size();

		auto textureIter = mTextureMap.find(fileName);

		if (textureIter == mTextureMap.end())
		{
			mTextureMap[fileName] = new Texture(std::move(fileName), device, descriptorManager);
			mTextureIndices.emplace_back(fileName);
		}

		return id;
	}

	bool TextureManager::Upload(
		WString&& fileName, SharedPtr<DeviceResources> device, ID3D12Resource* uploadHeap, ID3D12GraphicsCommandList* commandList)
	{
		auto textureIter = mTextureMap.find(fileName);

		if (textureIter != mTextureMap.end())
		{
			textureIter->second->Upload(device, uploadHeap, commandList);
		}
		else
		{
			throw Exception("Invalid Texture.");
		}

		return true;
	}

	bool TextureManager::Upload(
		UINT64 index, SharedPtr<DeviceResources> device, ID3D12Resource* uploadHeap, ID3D12GraphicsCommandList* commandList)
	{
		auto name = mTextureIndices.at(index);
		auto textureIter = mTextureMap.find(name);

		if (textureIter != mTextureMap.end())
		{
			textureIter->second->Upload(device, uploadHeap, commandList);
		}
		else
		{
			throw Exception("Invalid Texture.");
		}

		return true;
	}

	void TextureManager::UploadAll(SharedPtr<DeviceResources> device, SharedPtr<RenderSystem> renderer)
	{
		Vector<ID3D12Resource*> uploadHeaps;
		Vector<ID3D12GraphicsCommandList*> commandLists;
		Vector<ID3D12CommandAllocator*> commandAllocators;

		UINT64 size = mTextureMap.size();
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
		for (auto& item : mTextureMap)
		{
			auto& texture = item.second;
			const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(texture->GetUploadBufferSize());

			ID3D12Resource* uploadHeap = {};

			ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
				&uploadHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&uploadHeap)));

			uploadHeaps.emplace_back(uploadHeap);

#ifdef AMADEUS_CONCURRENCY
			results.emplace_back(renderer->Submit(&Texture::Upload, texture, device, uploadHeap, commandLists[i]));
#else
			texture->Upload(device, uploadHeap, commandLists[i]);
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

	void TextureManager::Unload(WString&& fileName)
	{
		auto textureIter = mTextureMap.find(fileName);

		if (textureIter != mTextureMap.end())
		{
			textureIter->second->Unload();
		}
	}

	void TextureManager::Unload(UINT64 index)
	{
		auto name = mTextureIndices.at(index);
		auto textureIter = mTextureMap.find(name);

		if (textureIter != mTextureMap.end())
		{
			textureIter->second->Unload();
		}
	}

	void TextureManager::UnloadAll()
	{
		for (auto& texture : mTextureMap)
		{
			texture.second->Unload();
		}
	}

	void TextureManager::Destroy()
	{
		for (auto& texture : mTextureMap)
		{
			texture.second->Destroy();
		}
		mTextureMap.clear();
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE TextureManager::GetDescriptorHandle(WString&& fileName)
	{
		auto textureIter = mTextureMap.find(fileName);

		if (textureIter != mTextureMap.end())
		{
			return textureIter->second->GetDescriptorHandle();
		}
		else
		{
			throw Exception("Invalid Texture.");
		}

		return {};
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE TextureManager::GetDescriptorHandle(UINT64 index)
	{
		auto name = mTextureIndices.at(index);
		auto textureIter = mTextureMap.find(name);

		if (textureIter != mTextureMap.end())
		{
			return textureIter->second->GetDescriptorHandle();
		}
		else
		{
			throw Exception("Invalid Texture.");
		}

		return {};
	}

	Texture* TextureManager::GetTexture(WString&& fileName)
	{
		auto textureIter = mTextureMap.find(fileName);

		if (textureIter != mTextureMap.end())
		{
			return textureIter->second;
		}
		else
		{
			throw Exception("Invalid Texture.");
		}

		return nullptr;
	}

	Texture* TextureManager::GetTexture(UINT64 index) 
	{
		auto name = mTextureIndices.at(index);
		auto textureIter = mTextureMap.find(name);

		if (textureIter != mTextureMap.end())
		{
			return textureIter->second;
		}
		else
		{
			throw Exception("Invalid Texture.");
		}

		return nullptr;
	}


}