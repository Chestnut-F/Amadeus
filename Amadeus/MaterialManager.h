#pragma once
#include "Prerequisites.h"
#include "Material.h"

namespace Amadeus
{
	class MaterialManager
	{
	public:
		static MaterialManager& Instance()
		{
			static MaterialManager* instance = new MaterialManager();
			return *instance;
		}

		UINT64 CreateMaterial(Material::TextureId baseColorId = -1, Material::TextureId metallicRoughnessId = -1,
			Material::TextureId normalId = -1, Material::TextureId occlusionId = -1, Material::TextureId emissiveId = -1);

		void SetPBRMetallicRoughness(UINT64 index, Vector<float>&& baseColorFactor, float metallicFactor, float roughnessFactor);

		void SetNormal(UINT64 index, float normalScale);

		void SetOcclusion(UINT64 index, float occlusionStrength);

		void SetEmissive(UINT64 index, Vector<float>&& emissiveFactor);

		void SetAlphaMode(UINT64 index, float alphaCutoff, Material::MATERIAL_ALPHA_MODE alphaMode);

		void SetDoubleSided(UINT64 index, bool bDouble);

		Material* GetMaterial(UINT64 index) { return mMaterialList.at(index); }

		void Destroy();

	private:
		MaterialManager() {};

		typedef Vector<Material*> MaterialList;
		MaterialList mMaterialList;
	};
}