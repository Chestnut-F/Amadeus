#include "pch.h"
#include "Primitive.h"
#include "MikkTSpace/mikktspace.h"
#include "MaterialManager.h"

namespace Amadeus
{
    static constexpr int TRIANGLE_VERTEX_COUNT = 3;

    Primitive::Primitive(
        Vector<Vertex>&& vertices, 
        Vector<uint32_t>&& indices, 
        XMMATRIX modelMatrix, 
        INT material, 
        bool normalsProvided, 
        bool tangentsProvided, 
        D3D12_PRIMITIVE_TOPOLOGY mode)
        : mVertices(std::move(vertices))
        , mIndices(std::move(indices))
        , mMaterialId(material)
        , mMode(mode)
    {
        if (!normalsProvided)
        {
            ComputeTriangleNormals();
        }

        if (!tangentsProvided)
        {
            ComputeTriangleTangents();
        }

        XMStoreFloat4x4(&mPrimitiveConstantBuffer.model, modelMatrix);
        if (mMaterialId > -1)
            SetMaterial();
    }

    bool Primitive::Upload(
        SharedPtr<DeviceResources> device, 
        ID3D12Resource* verticesUploadHeap, 
        ID3D12Resource* indicesUploadHeap, 
        ID3D12GraphicsCommandList* commandList)
    {
        UploadVertices(device, verticesUploadHeap, commandList);

        UploadIndices(device, indicesUploadHeap, commandList);

        const CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(mPrimitiveConstantBufferSize);

        ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mPrimitiveConstants)));
        NAME_D3D12_OBJECT(mPrimitiveConstants);

        // Map and initialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay.
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(mPrimitiveConstants->Map(0, &readRange, reinterpret_cast<void**>(&pPrimitiveCbvDataBegin)));
        memcpy(pPrimitiveCbvDataBegin, &mPrimitiveConstantBuffer, mPrimitiveConstantBufferSize);
        mPrimitiveConstants->Unmap(0, 0);

        mMaterial->Upload(device);

        commandList->Close();

        ID3D12CommandList* ppCommandLists[] = { commandList };
        device->GetCommandQueue()->ExecuteCommandLists(1, ppCommandLists);

        return true;
    }

    void Primitive::Render(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache, ID3D12GraphicsCommandList* commandList)
    {
        commandList->IASetIndexBuffer(&mIndexBufferView);
        commandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
        commandList->SetGraphicsRootConstantBufferView(COMMON_PRIMITIVE_ROOT_CBV_INDEX, mPrimitiveConstants->GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(COMMON_MATERIAL_ROOT_CBV_INDEX, mMaterial->GetD3D12Resource()->GetGPUVirtualAddress());

        mMaterial->Render(device, descriptorCache, commandList);

        commandList->DrawIndexedInstanced(mNumIndices, 1, 0, 0, 0);
    }

    void Primitive::SetMaterial()
    {
        mMaterial = MaterialManager::Instance().GetMaterial(mMaterialId);
    }

    Material* Primitive::GetMaterial() const
    {
        return mMaterial;
    }

    D3D12_CONSTANT_BUFFER_VIEW_DESC Primitive::GetCbvDesc(SharedPtr<DeviceResources> device)
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = mPrimitiveConstants->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = mPrimitiveConstantBufferSize;
        return std::move(cbvDesc);
    }

    void Primitive::ComputeTriangleNormals()
    {
        assert((mIndices.size() % TRIANGLE_VERTEX_COUNT) == 0); // Only triangles are supported.

        // Loop through each triangle
        for (uint32_t i = 0; i < mIndices.size(); i += TRIANGLE_VERTEX_COUNT)
        {
            // References to the three vertices of the triangle.
            Vertex& v0 = mVertices[mIndices[i]];
            Vertex& v1 = mVertices[mIndices[i + 1]];
            Vertex& v2 = mVertices[mIndices[i + 2]];

            // Compute normal. Normalization happens later.
            const XMVECTOR pos0 = XMLoadFloat3(&v0.position);
            const XMVECTOR d0 = XMVectorSubtract(XMLoadFloat3(&v2.position), pos0);
            const XMVECTOR d1 = XMVectorSubtract(XMLoadFloat3(&v1.position), pos0);
            const XMVECTOR normal = XMVector3Cross(d0, d1);

            // Add the normal to the three vertices of the triangle. Normals are added
            // so that reused vertices will get the average normal (done later).
            // Note that the normals are not normalized at this point, so larger triangles
            // will have more weight than small triangles which share a vertex. This
            // appears to give better results.
            XMStoreFloat3(&v0.normal, XMVectorAdd(XMLoadFloat3(&v0.normal), normal));
            XMStoreFloat3(&v1.normal, XMVectorAdd(XMLoadFloat3(&v1.normal), normal));
            XMStoreFloat3(&v2.normal, XMVectorAdd(XMLoadFloat3(&v2.normal), normal));
        }

        // Since the same vertex may have been used by multiple triangles, and the cross product normals
        // aren't normalized yet, normalize the computed normals.
        for (Vertex& vertex : mVertices)
        {
            XMStoreFloat3(&vertex.normal, XMVector3Normalize(XMLoadFloat3(&vertex.normal)));
        }
    }

    void Primitive::ComputeTriangleTangents()
    {
        // Set up the callbacks so that MikkTSpace can read the Primitive data.
        SMikkTSpaceInterface mikkInterface{};
        mikkInterface.m_getNumFaces = [](const SMikkTSpaceContext* pContext) {
            auto primitive = static_cast<const Primitive*>(pContext->m_pUserData);
            assert((primitive->mIndices.size() % TRIANGLE_VERTEX_COUNT) == 0); // Only triangles are supported.
            return (int)(primitive->mIndices.size() / TRIANGLE_VERTEX_COUNT);
        };
        mikkInterface.m_getNumVerticesOfFace = [](const SMikkTSpaceContext* pContext, int iFace) {
            return TRIANGLE_VERTEX_COUNT;
        };
        mikkInterface.m_getPosition = [](const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert) {
            auto primitive = static_cast<const Primitive*>(pContext->m_pUserData);
            const auto vertexIndex = primitive->mIndices[(iFace * TRIANGLE_VERTEX_COUNT) + iVert];
            memcpy(fvPosOut, &primitive->mVertices[vertexIndex].position, sizeof(float) * 3);
        };
        mikkInterface.m_getNormal = [](const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert) {
            auto primitive = static_cast<const Primitive*>(pContext->m_pUserData);
            const auto vertexIndex = primitive->mIndices[(iFace * TRIANGLE_VERTEX_COUNT) + iVert];
            memcpy(fvNormOut, &primitive->mVertices[vertexIndex].normal, sizeof(float) * 3);
        };
        mikkInterface.m_getTexCoord = [](const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert) {
            auto primitive = static_cast<const Primitive*>(pContext->m_pUserData);
            const auto vertexIndex = primitive->mIndices[(iFace * TRIANGLE_VERTEX_COUNT) + iVert];
            memcpy(fvTexcOut, &primitive->mVertices[vertexIndex].texCoord0, sizeof(float) * 2);
        };
        mikkInterface.m_setTSpaceBasic = [](const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert) {
            auto primitive = static_cast<Primitive*>(pContext->m_pUserData);
            const auto vertexIndex = primitive->mIndices[(iFace * TRIANGLE_VERTEX_COUNT) + iVert];
            primitive->mVertices[vertexIndex].tangent.x = fvTangent[0];
            primitive->mVertices[vertexIndex].tangent.y = fvTangent[1];
            primitive->mVertices[vertexIndex].tangent.z = fvTangent[2];
            primitive->mVertices[vertexIndex].tangent.w = fSign;
        };

        // Run the MikkTSpace algorithm.
        SMikkTSpaceContext mikkContext{};
        mikkContext.m_pUserData = this;
        mikkContext.m_pInterface = &mikkInterface;
        if (genTangSpaceDefault(&mikkContext) == 0)
        {
            throw Exception("Failed to generate tangents");
        }
    }

    void Primitive::UploadVertices(SharedPtr<DeviceResources> device, ID3D12Resource* uploadHeap, ID3D12GraphicsCommandList* commandList)
    {
        UINT vertexDataSize = static_cast<UINT>(mVertices.size() * sizeof(Vertex));
        const CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexDataSize);

        ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&mVertexBuffer)));
        NAME_D3D12_OBJECT(mVertexBuffer);

        D3D12_SUBRESOURCE_DATA vertexData = {};
        vertexData.pData = mVertices.data();
        vertexData.RowPitch = vertexDataSize;
        vertexData.SlicePitch = vertexData.RowPitch;

        UpdateSubresources<1>(commandList, mVertexBuffer.Get(), uploadHeap, 0, 0, 1, &vertexData);
        CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            mVertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        commandList->ResourceBarrier(1, &resourceBarrier);

        mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
        mVertexBufferView.StrideInBytes = static_cast<UINT>(sizeof(Vertex));
        mVertexBufferView.SizeInBytes = vertexDataSize;
    }

    void Primitive::UploadIndices(SharedPtr<DeviceResources> device, ID3D12Resource* uploadHeap, ID3D12GraphicsCommandList* commandList)
    {
        UINT indexDataSize = static_cast<UINT>(mIndices.size() * sizeof(UINT32));
        const CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexDataSize);

        ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&mIndexBuffer)));
        NAME_D3D12_OBJECT(mIndexBuffer);

        D3D12_SUBRESOURCE_DATA indexData = {};
        indexData.pData = mIndices.data();
        indexData.RowPitch = indexDataSize;
        indexData.SlicePitch = indexDataSize;

        UpdateSubresources<1>(commandList, mIndexBuffer.Get(), uploadHeap, 0, 0, 1, &indexData);
        CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            mIndexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
        commandList->ResourceBarrier(1, &resourceBarrier);

        // Describe the index buffer view.
        mIndexBufferView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
        mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
        mIndexBufferView.SizeInBytes = indexDataSize;

        mNumIndices = indexDataSize / 4;    // R32_UINT (SampleAssets::StandardIndexFormat) = 4 bytes each.
    }

    void Primitive::UploadMaterial(SharedPtr<DeviceResources> device)
    {
        mMaterial->Upload(device);
    }
}