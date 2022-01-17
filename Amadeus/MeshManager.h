#pragma once
#include "Prerequisites.h"
#include "Mesh.h"

namespace Amadeus
{
	class MeshManager : public Observer
	{
	public:
		MeshManager(const MeshManager&) = delete;
		MeshManager& operator=(const MeshManager&) = delete;

		static MeshManager& Instance()
		{
			static MeshManager* instance = new MeshManager();
			return *instance;
		}

		void Init();

		void Destroy();

		UINT64 Size() { return mMeshList.size(); }

		UINT64 CreateMesh(XMMATRIX modelMatrix);

		void UploadAll(SharedPtr<DeviceResources> device, SharedPtr<RenderSystem> renderer);

		void Render(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache, ID3D12GraphicsCommandList* commandList);

		Mesh* GetMesh(UINT64 index) { return mMeshList.at(index); }

		const Vector<Mesh*>& GetMeshList() { return mMeshList; }

	private:
		MeshManager() {}

		typedef Vector<Mesh*> MeshList;
		MeshList mMeshList;
	};
}