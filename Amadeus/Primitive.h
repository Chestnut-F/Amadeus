#pragma once
#include "Prerequisites.h"

namespace Amadeus
{
	class Material;

	class Primitive
	{
	public:
		struct Vertex
		{
			XMFLOAT3 position;
			XMFLOAT3 normal;
			XMFLOAT4 tangent;
			XMFLOAT2 texCoord0;
			// Note: This implementation does not currently support glTF 2's Color0 and TexCoord1 attributes.
		};

		struct PrimitiveConstantBuffer
		{
			XMFLOAT4X4 model;
			float padding[48];
		};
		static_assert((sizeof(PrimitiveConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

	public:
		Primitive(const Primitive&) = delete;
		Primitive& operator=(const Primitive&) = delete;

		explicit Primitive(
			Vector<Vertex>&& vertices, 
			Vector<UINT>&& indices, 
			XMMATRIX modelMatrix = XMMatrixIdentity(),
			INT material = -1,
			bool normalsProvided = true, 
			bool tangentsProvided = true, 
			D3D12_PRIMITIVE_TOPOLOGY mode = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		~Primitive() = default;

		bool Upload(
			SharedPtr<DeviceResources> device, 
			ID3D12Resource* verticesUploadHeap, 
			ID3D12Resource* indicesUploadHeap, 
			ID3D12GraphicsCommandList* commandList);

		void Render(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache, ID3D12GraphicsCommandList* commandList);

		void SetModelMatrix(XMFLOAT4X4 model) { mPrimitiveConstantBuffer.model = model; }

		void SetMaterial();

		Material* GetMaterial() const;

		UINT GetVertexDataSize() { return static_cast<UINT>(mVertices.size() * sizeof(Vertex)); }

		UINT GetIndexDataSize() { return static_cast<UINT>(mIndices.size() * sizeof(UINT)); }

		const D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView() const { return &mVertexBufferView; }

		const D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView() const { return &mIndexBufferView; }

		UINT GetNumIndices() const { return mNumIndices; }

		D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveMode() const { return mMode; }

		D3D12_CONSTANT_BUFFER_VIEW_DESC GetCbvDesc(SharedPtr<DeviceResources> device);

	private:
		typedef D3D12_PRIMITIVE_TOPOLOGY PrimitiveMode;
		PrimitiveMode mMode;

		INT mMaterialId;
		Material* mMaterial;

		Vector<Vertex> mVertices;
		ComPtr<ID3D12Resource> mVertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;

		Vector<UINT> mIndices;
		ComPtr<ID3D12Resource> mIndexBuffer;
		D3D12_INDEX_BUFFER_VIEW mIndexBufferView;

		ComPtr<ID3D12Resource> mPrimitiveConstants;
		PrimitiveConstantBuffer mPrimitiveConstantBuffer;
		UINT8* pPrimitiveCbvDataBegin;
		const UINT mPrimitiveConstantBufferSize = sizeof(PrimitiveConstantBuffer);

		UINT mNumIndices;

		void ComputeTriangleNormals();

		void ComputeTriangleTangents();

		void UploadVertices(SharedPtr<DeviceResources> device, ID3D12Resource* uploadHeap, ID3D12GraphicsCommandList* commandList);

		void UploadIndices(SharedPtr<DeviceResources> device, ID3D12Resource* uploadHeap, ID3D12GraphicsCommandList* commandList);

		void UploadMaterial(SharedPtr<DeviceResources> device);
	};
}