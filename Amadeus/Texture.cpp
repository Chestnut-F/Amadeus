#include "pch.h"
#include "Texture.h"

namespace Amadeus
{
    Texture::Texture(WString&& fileName, SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager)
    {
        CreateFromFile(std::move(fileName));

        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.MipLevels = mMetadata.mipLevels;
        textureDesc.Format = mMetadata.format;
        textureDesc.Width = mMetadata.width;
        textureDesc.Height = mMetadata.height;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        textureDesc.DepthOrArraySize = mMetadata.arraySize;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(mMetadata.dimension);

        const CD3DX12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&mTextureResource)));
        NAME_D3D12_OBJECT(mTextureResource);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = mMetadata.format;
        if (mMetadata.IsCubemap()) {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray.MipLevels = mMetadata.mipLevels;
            srvDesc.Texture2DArray.ArraySize = mMetadata.arraySize;
        }
        else {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = mMetadata.mipLevels;
        }
        mHandle = descriptorManager->AllocateSrvHeap(device, mTextureResource.Get(), srvDesc);

        LoadFromFile(std::move(fileName));
    }

    Texture::Texture(Vector<UINT8>&& image, SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager)
    {
        CreateFromMemory(std::move(image));

        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.MipLevels = mMetadata.mipLevels;
        textureDesc.Format = mMetadata.format;
        textureDesc.Width = mMetadata.width;
        textureDesc.Height = mMetadata.height;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        textureDesc.DepthOrArraySize = mMetadata.arraySize;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(mMetadata.dimension);

        const CD3DX12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&mTextureResource)));
        NAME_D3D12_OBJECT(mTextureResource);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = mMetadata.format;
        if (mMetadata.IsCubemap()) {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray.MipLevels = mMetadata.mipLevels;
            srvDesc.Texture2DArray.ArraySize = mMetadata.arraySize;
        }
        else {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = mMetadata.mipLevels;
        }
        mHandle = descriptorManager->AllocateSrvHeap(device, mTextureResource.Get(), srvDesc);

        LoadFromMemory(std::move(image));
    }

    bool Texture::Upload(SharedPtr<DeviceResources> device, ID3D12Resource* uploadHeap, ID3D12GraphicsCommandList* commandList)
    {
        //commandList->Reset(device->GetCommandAllocator(), nullptr);

        const auto& images = mImage.GetImages();
        D3D12_SUBRESOURCE_DATA textureData = {};
        textureData.pData = mImage.GetPixels();
        textureData.RowPitch = images->rowPitch;
        textureData.SlicePitch = images->slicePitch;

        UpdateSubresources<1>(commandList, mTextureResource.Get(), uploadHeap, 0, 0, 1, &textureData);

        const CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            mTextureResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, &resourceBarrier);

        commandList->Close();

        ID3D12CommandList* ppCommandLists[] = { commandList };
        device->GetCommandQueue()->ExecuteCommandLists(1, ppCommandLists);

        return true;
    }

    void Texture::Unload()
    {
        mImage.Release();
    }

    void Texture::Destroy()
    {
        mTextureResource->Release();
    }

    void Texture::CreateFromFile(WString&& fileName)
    {
        WString fullPath = GetAssetFullPath(L"Assets\\" + fileName);
        WString suffix = WString(fileName, fileName.find(L"."));
        std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::towupper);

        if (suffix == L".BMP" || suffix == L".PNG" || suffix == L".GIF" || suffix == L".TIFF" || suffix == L".JPEG" || suffix == L".JPG") {
            ThrowIfFailed(GetMetadataFromWICFile(fullPath.c_str(), WIC_FLAGS_DEFAULT_SRGB, mMetadata));
        }
        else if (suffix == L".DDS") {
            ThrowIfFailed(GetMetadataFromDDSFile(fullPath.c_str(), DDS_FLAGS_NONE, mMetadata));
        }
    }

    void Texture::CreateFromMemory(Vector<UINT8>&& image)
    {
        ThrowIfFailed(GetMetadataFromWICMemory(image.data(), image.size(), WIC_FLAGS_NONE, mMetadata));
    }

    void Texture::LoadFromFile(WString&& fileName)
    {
        WString fullPath = GetAssetFullPath(L"Assets\\" + fileName);
        WString suffix = WString(fileName, fileName.find(L"."));
        std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::towupper);

        if (suffix == L".BMP" || suffix == L".PNG" || suffix == L".GIF" || suffix == L".TIFF" || suffix == L".JPEG" || suffix == L".JPG") {
            ThrowIfFailed(LoadFromWICFile(fullPath.c_str(), WIC_FLAGS_DEFAULT_SRGB, nullptr, mImage));
        }
        else if (suffix == L".DDS") {
            ThrowIfFailed(LoadFromDDSFile(fullPath.c_str(), DDS_FLAGS_NONE, nullptr, mImage));
        }
    }

    void Texture::LoadFromMemory(Vector<UINT8>&& image)
    {
        ThrowIfFailed(LoadFromWICMemory(image.data(), image.size(), WIC_FLAGS_NONE, nullptr, mImage));
    }


}