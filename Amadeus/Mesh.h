#pragma once
#include "Prerequisites.h"
#include "Primitive.h"

namespace Amadeus
{
	class Mesh
	{
	public:
	public:
		explicit Mesh(XMMATRIX modelMatrix)
		{
			XMStoreFloat4x4(&mModelMatrix, XMMatrixTranspose(modelMatrix));
		}

		~Mesh() = default;

		UINT64 CreatrPrimitive(Vector<Primitive::Vertex>&& vertices, Vector<UINT>&& indices, INT material = -1,
			bool normalsProvided = true, bool tangentsProvided = true, D3D12_PRIMITIVE_TOPOLOGY mode = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		Vector<Primitive*>& GetPrimitives() { return mPrimitiveList; }

		Primitive* GetPrimitive(UINT64 index) { return mPrimitiveList.at(index); }

		UINT64 GetPrimitiveSize() { return mPrimitiveList.size(); }

		void UploadAll(SharedPtr<DeviceResources> device, SharedPtr<RenderSystem> renderer);

		void RenderShadow(SharedPtr<DeviceResources> device, 
			SharedPtr<DescriptorCache> descriptorCache, ID3D12GraphicsCommandList* commandList);

		void Render(SharedPtr<DeviceResources> device, 
			SharedPtr<DescriptorCache> descriptorCache, ID3D12GraphicsCommandList* commandList);
		
		void Destroy();

		const Boundary& GetBoundary() { return mBoundary; }

	private:
		typedef Vector<Primitive*> PrimitiveList;
		PrimitiveList mPrimitiveList;

		XMFLOAT4X4 mModelMatrix;

		Boundary mBoundary;

		void StatBoundary(Primitive* primitive);
	};
}