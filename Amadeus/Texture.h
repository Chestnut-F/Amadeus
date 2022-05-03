#pragma once
#include "Prerequisites.h"

namespace Amadeus
{
    enum class TextureType
    {
        BASE_COLOR,
        CUBE_MAP,
        OTHER,
        DEFAULT,
        NUM_TEXTURE_TYPE,
    };

	class Texture
	{
    public:
        Texture(WString&& fileName, TextureType type, SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager);

        Texture(Vector<UINT8>&& image, TextureType type, SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager);

        UINT64 GetUploadBufferSize() { return GetRequiredIntermediateSize(mTextureResource.Get(), 0, 1); }

        bool Upload(SharedPtr<DeviceResources> device, ID3D12Resource* uploadHeap, ID3D12GraphicsCommandList* commandList);

        bool PreCompute(ResourceUploadBatch& uploadBatch);

        void Unload();

        void Destroy();

        CD3DX12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle();

        void SetFiltered(bool filtered) { bFiltered = filtered; }

        TextureType GetType() { return mType; }

	private:
        void CreateFromFile(WString&& fileName);

        void CreateFromMemory(Vector<UINT8>&& image);

        void LoadFromFile(WString&& fileName);

        void LoadFromMemory(Vector<UINT8>&& image);

        UINT16 GetMipLevels();

        TextureType mType;

        TexMetadata mMetadata;
		ScratchImage mImage;

		ComPtr<ID3D12Resource> mTextureResource;
		CD3DX12_CPU_DESCRIPTOR_HANDLE mHandle;

        bool bFiltered = true;
        WString mName;

        // Cube Map
        ComPtr<ID3D12Resource> mUploadHeap;
        Vector<D3D12_SUBRESOURCE_DATA> mSubresources;
	};
}