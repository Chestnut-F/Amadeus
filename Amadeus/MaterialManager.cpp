#include "pch.h"
#include "MaterialManager.h"

namespace Amadeus
{
	UINT64 MaterialManager::CreateMaterial(Material::TextureId baseColorId, Material::TextureId metallicRoughnessId, 
		Material::TextureId normalId, Material::TextureId occlusionId, Material::TextureId emissiveId)
	{
		UINT64 id = mMaterialList.size();
		Material* material = new Material(baseColorId, metallicRoughnessId, normalId, occlusionId, emissiveId);
		mMaterialList.emplace_back(material);
		return id;
	}

	void MaterialManager::SetPBRMetallicRoughness(
		UINT64 index, Vector<float>&& baseColorFactor, float metallicFactor, float roughnessFactor)
	{
		mMaterialList.at(index)
			->SetPBRMetallicRoughness(std::move(baseColorFactor), metallicFactor, roughnessFactor);
	}

	void MaterialManager::SetNormal(UINT64 index, float normalScale)
	{
		mMaterialList.at(index)
			->SetNormal(normalScale);
	}

	void MaterialManager::SetOcclusion(UINT64 index, float occlusionStrength)
	{
		mMaterialList.at(index)
			->SetOcclusion(occlusionStrength);
	}

	void MaterialManager::SetEmissive(UINT64 index, Vector<float>&& emissiveFactor)
	{
		mMaterialList.at(index)
			->SetEmissive(std::move(emissiveFactor));
	}

	void MaterialManager::SetAlphaMode(UINT64 index, float alphaCutoff, Material::MATERIAL_ALPHA_MODE alphaMode)
	{
		mMaterialList.at(index)
			->SetAlphaMode(alphaCutoff, alphaMode);
	}

	void MaterialManager::SetDoubleSided(UINT64 index, bool bDouble)
	{
		mMaterialList.at(index)
			->SetDoubleSided(bDouble);
	}

	void MaterialManager::Destroy()
	{
		for (auto& material : mMaterialList)
		{
			material->Destroy();
		}
		mMaterialList.clear();
	}
}