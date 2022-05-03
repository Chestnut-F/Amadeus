#include "pch.h"
#include "Texture.h"

namespace Amadeus
{
    Texture::Texture(WString&& fileName, TextureType type, SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager)
        : bFiltered(true)
        , mType(type)
        , mName(fileName)
    {
        CreateFromFile(std::move(fileName));

        if (mType == TextureType::BASE_COLOR && !mMetadata.IsCubemap() && mMetadata.mipLevels == 1)
        {
            bFiltered = false;
            mMetadata.mipLevels = GetMipLevels();
        }

        // 是否支持自动生成Mipmaps
        ResourceUploadBatch upload(device->GetD3DDevice());
        if (!upload.IsSupportedForGenerateMips(mMetadata.format))
        {
            bFiltered = true;
            mMetadata.mipLevels = 1;
        }

        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.MipLevels = mMetadata.mipLevels;
        textureDesc.Format = mMetadata.format;
        textureDesc.Width = mMetadata.width;
        textureDesc.Height = mMetadata.height;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        textureDesc.DepthOrArraySize = mMetadata.arraySize;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

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
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            srvDesc.TextureCube.MipLevels = mMetadata.mipLevels;
        }
        else {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = mMetadata.mipLevels;
        }
        mHandle = descriptorManager->AllocateSrvHeap(device, mTextureResource.Get(), srvDesc);

        LoadFromFile(std::move(fileName));
    }

    Texture::Texture(Vector<UINT8>&& image, TextureType type, SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager)
        : bFiltered(true)
        , mType(type)
    {
        CreateFromMemory(std::move(image));

        if (mType == TextureType::BASE_COLOR && !mMetadata.IsCubemap() && mMetadata.mipLevels == 1)
        {
            bFiltered = false;
            mMetadata.mipLevels = GetMipLevels();
        }

        // 是否支持自动生成Mipmaps
        ResourceUploadBatch upload(device->GetD3DDevice());
        if (!upload.IsSupportedForGenerateMips(mMetadata.format))
        {
            bFiltered = true;
            mMetadata.mipLevels = 1;
        }

        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.MipLevels = mMetadata.mipLevels;
        textureDesc.Format = mMetadata.format;
        textureDesc.Width = mMetadata.width;
        textureDesc.Height = mMetadata.height;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        textureDesc.DepthOrArraySize = mMetadata.arraySize;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

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
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            srvDesc.TextureCube.MipLevels = mMetadata.mipLevels;
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

        if (mMetadata.IsCubemap())
        {
            ThrowIfFailed(PrepareUpload(device->GetD3DDevice(), mImage.GetImages(), mImage.GetImageCount(), mMetadata, mSubresources));
            UINT64 uploadBufferSize = GetRequiredIntermediateSize(mTextureResource.Get(), 0, static_cast<UINT>(mSubresources.size()));

            const CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

            ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
                &uploadHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&mUploadHeap)));

            UpdateSubresources(commandList, mTextureResource.Get(), mUploadHeap.Get(), 0, 0, mSubresources.size(), mSubresources.data());
        }
        else
        {
            const auto& images = mImage.GetImages();
            D3D12_SUBRESOURCE_DATA textureData = {};
            textureData.pData = mImage.GetPixels();
            textureData.RowPitch = images->rowPitch;
            textureData.SlicePitch = images->slicePitch;

            UpdateSubresources<1>(commandList, mTextureResource.Get(), uploadHeap, 0, 0, 1, &textureData);
        }

        const CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            mTextureResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, &resourceBarrier);

        commandList->Close();

        ID3D12CommandList* ppCommandLists[] = { commandList };
        device->GetCommandQueue()->ExecuteCommandLists(1, ppCommandLists);

        return true;
    }

    bool Texture::PreCompute(ResourceUploadBatch& uploadBatch)
    {
        if (bFiltered) return bFiltered;
        assert(!mMetadata.IsCubemap());

        if (uploadBatch.IsSupportedForGenerateMips(mMetadata.format))
        {
            uploadBatch.GenerateMips(mTextureResource.Get());
        }

        return true;
    }

    void Texture::Unload()
    {
        mImage.Release();
    }

    void Texture::Destroy()
    {
        if (mMetadata.IsCubemap())
        {
            mUploadHeap->Release();
            mSubresources.clear();
        }

        mTextureResource->Release();
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE Texture::GetDescriptorHandle() 
    { 
        assert(bFiltered);
        return mHandle; 
    }

    void Texture::CreateFromFile(WString&& fileName)
    {
        WString fullPath = GetAssetFullPath(L"..\\..\\Assets\\" + fileName);
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
        WString fullPath = GetAssetFullPath(L"..\\..\\Assets\\" + fileName);
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

    UINT16 Texture::GetMipLevels()
    {
        UINT16 mipLevels = 1;
        size_t size = mMetadata.width;
        while (size >> 1)
        {
            size >>= 1;
            mipLevels++;
        }
        return mipLevels;
    }

}