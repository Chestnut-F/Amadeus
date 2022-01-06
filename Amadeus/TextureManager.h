#pragma once
#include "Prerequisites.h"
#include "Texture.h"
#include "RenderSystem.h"

namespace Amadeus
{
	class TextureManager
	{
	public:
		static TextureManager& Instance()
		{
			static TextureManager* instance = new TextureManager();
			return *instance;
		}

		UINT64 LoadFromFile(WString&& fileName, SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager);

		bool Upload(WString&& fileName, SharedPtr<DeviceResources> device, ID3D12Resource* uploadHeap, ID3D12GraphicsCommandList* commandList);

		bool Upload(UINT64 index, SharedPtr<DeviceResources> device, ID3D12Resource* uploadHeap, ID3D12GraphicsCommandList* commandList);

		void UploadAll(SharedPtr<DeviceResources> device, SharedPtr<RenderSystem> renderer);

		void Unload(WString&& fileName);

		void Unload(UINT64 index);

		void UnloadAll();

		CD3DX12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(WString&& fileName);

		CD3DX12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(UINT64 index);

		Texture* GetTexture(WString&& fileName);

		Texture* GetTexture(UINT64 index);

		bool Empty()
		{
			assert(mTextureIndices.size() == mTextureMap.size());
			return mTextureIndices.empty();
		}

	private:
		TextureManager() {}

		typedef Map<WString, Texture*> TextureMap;
		Vector<WString> mTextureIndices;
		TextureMap mTextureMap;
	};
}