#pragma once
#include "Prerequisites.h"

namespace Amadeus
{
    static constexpr wchar_t TEXTURE_EMPTY_ID[19] = L"Textures\\white.png";

	class Texture
	{
	public:
        Texture(WString&& fileName, SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager);

        Texture(Vector<UINT8>&& image, SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager);

        UINT64 GetUploadBufferSize() { return GetRequiredIntermediateSize(mTextureResource.Get(), 0, 1); }

        bool Upload(SharedPtr<DeviceResources> device, ID3D12Resource* uploadHeap, ID3D12GraphicsCommandList* commandList);

        void Unload();

        CD3DX12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() { return mHandle; }

	private:
        void CreateFromFile(WString&& fileName);

        void CreateFromMemory(Vector<UINT8>&& image);

        void LoadFromFile(WString&& fileName);

        void LoadFromMemory(Vector<UINT8>&& image);

        TexMetadata mMetadata;
		ScratchImage mImage;

		ComPtr<ID3D12Resource> mTextureResource;
		CD3DX12_CPU_DESCRIPTOR_HANDLE mHandle;
	};
}