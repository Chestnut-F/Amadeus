#include "pch.h"
#include "Material.h"
#include "TextureManager.h"

namespace Amadeus
{
	Material::Material(TextureId baseColorId, TextureId metallicRoughnessId, 
		TextureId normalId, TextureId occlusionId, TextureId emissiveId)
		: mBaseColorId(baseColorId)
		, mMetallicRoughnessId(metallicRoughnessId)
		, mNormalId(normalId)
		, mOcclusionId(occlusionId)
		, mEmissiveId(emissiveId)
	{
		if (mBaseColorId > -1)
			mType = mType | MATERIAL_TYPE_BASECOLOR;
		if (mMetallicRoughnessId > -1)
			mType = mType | MATERIAL_TYPE_METALLIC_ROUGHNESS;
		if (mNormalId > -1)
			mType = mType | MATERIAL_TYPE_NORMAL;
		if (mOcclusionId > -1)
			mType = mType | MATERIAL_TYPE_OCCLUSION;
		if (mEmissiveId > -1)
			mType = mType | MATERIAL_TYPE_EMISSIVE;
	}

	bool Material::SetPBRMetallicRoughness(Vector<float>&& baseColorFactor, float metallicFactor, float roughnessFactor)
	{
		if (mBaseColorId < 0 || mMetallicRoughnessId < 0)
			return false;

		mBaseColorFactor.swap(baseColorFactor);
		mMetallicFactor = metallicFactor;
		mRoughnessFactor = roughnessFactor;

		mBaseColor = TextureManager::Instance().GetTexture(mBaseColorId);

		bInitialized = bInitialized | MATERIAL_TYPE_BASECOLOR;
		
		mMetallicRoughness = TextureManager::Instance().GetTexture(mMetallicRoughnessId);

		bInitialized = bInitialized | MATERIAL_TYPE_METALLIC_ROUGHNESS;

		mMaterialConstantBuffer.baseColorFactor = XMFLOAT4(mBaseColorFactor.data());
		mMaterialConstantBuffer.metallicFactor = mMetallicFactor;
		mMaterialConstantBuffer.roughnessFactor = mRoughnessFactor;

		return true;
	}

	bool Material::SetNormal(float normalScale)
	{
		if (mNormalId < 0)
			return false;

		mNormalScale = normalScale;

		mNormal = TextureManager::Instance().GetTexture(mNormalId);

		bInitialized = bInitialized | MATERIAL_TYPE_NORMAL;

		mMaterialConstantBuffer.normalScale = mNormalScale;

		return true;
	}

	bool Material::SetOcclusion(float occlusionStrength)
	{
		if (mOcclusionId < 0)
			return false;

		mOcclusionStrength = occlusionStrength;

		mOcclusion = TextureManager::Instance().GetTexture(mOcclusionId);

		bInitialized = bInitialized | MATERIAL_TYPE_OCCLUSION;

		mMaterialConstantBuffer.occlusionStrength = mOcclusionStrength;

		return true;
	}

	bool Material::SetEmissive(Vector<float>&& emissiveFactor)
	{
		if (mEmissiveId < 0)
			return false;

		mEmissiveFactor.swap(emissiveFactor);

		mEmissive = TextureManager::Instance().GetTexture(mEmissiveId);

		bInitialized = bInitialized | MATERIAL_TYPE_EMISSIVE;

		mMaterialConstantBuffer.emissiveFactor = XMFLOAT3(mEmissiveFactor.data());

		return true;
	}

	void Material::SetAlphaMode(float alphaCutoff, MATERIAL_ALPHA_MODE alphaMode)
	{
		mAlphaCutoff = alphaCutoff;
		mAlphaMode = alphaMode;
	}

	void Material::SetDoubleSided(bool bDouble)
	{
		bDoubleSided = bDouble;
	}

	bool Material::Upload(SharedPtr<DeviceResources> device)
	{
		mMaterialConstantBuffer.materialType = mType;

		const CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(mMaterialConstantBufferSize);

		ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mMaterialConstants)));
		NAME_D3D12_OBJECT(mMaterialConstants);

		// Map and initialize the constant buffer. We don't unmap this until the
		// app closes. Keeping things mapped for the lifetime of the resource is okay.
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(mMaterialConstants->Map(0, &readRange, reinterpret_cast<void**>(&pMaterialCbvDataBegin)));
		memcpy(pMaterialCbvDataBegin, &mMaterialConstantBuffer, mMaterialConstantBufferSize);
		mMaterialConstants->Unmap(0, 0);

		return true;
	}

	void Material::Render(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache, ID3D12GraphicsCommandList* commandList)
	{
		Texture* whiteTexture = TextureManager::Instance().GetTexture(TEXTURE_EMPTY_ID);

		CD3DX12_GPU_DESCRIPTOR_HANDLE materialHandle = {};
		if (mType & MATERIAL_TYPE_BASECOLOR
			&& bInitialized & MATERIAL_TYPE_BASECOLOR)
		{
			materialHandle = descriptorCache->AppendSrvCache(device, mBaseColor->GetDescriptorHandle());
		}
		else
		{
			materialHandle = descriptorCache->AppendSrvCache(device, whiteTexture->GetDescriptorHandle());
		}

		if (mType & MATERIAL_TYPE_METALLIC_ROUGHNESS
			&& bInitialized & MATERIAL_TYPE_METALLIC_ROUGHNESS)
		{
			descriptorCache->AppendSrvCache(device, mMetallicRoughness->GetDescriptorHandle());
		}
		else
		{
			descriptorCache->AppendSrvCache(device, whiteTexture->GetDescriptorHandle());
		}

		if (mType & MATERIAL_TYPE_NORMAL
			&& bInitialized & MATERIAL_TYPE_NORMAL)
		{
			descriptorCache->AppendSrvCache(device, mNormal->GetDescriptorHandle());
		}
		else
		{
			descriptorCache->AppendSrvCache(device, whiteTexture->GetDescriptorHandle());
		}

		if (mType & MATERIAL_TYPE_OCCLUSION
			&& bInitialized & MATERIAL_TYPE_OCCLUSION)
		{
			descriptorCache->AppendSrvCache(device, mOcclusion->GetDescriptorHandle());
		}
		else
		{
			descriptorCache->AppendSrvCache(device, whiteTexture->GetDescriptorHandle());
		}

		if (mType & MATERIAL_TYPE_EMISSIVE
			&& bInitialized & MATERIAL_TYPE_EMISSIVE)
		{
			descriptorCache->AppendSrvCache(device, mEmissive->GetDescriptorHandle());
		}
		else
		{
			descriptorCache->AppendSrvCache(device, whiteTexture->GetDescriptorHandle());
		}

		commandList->SetGraphicsRootDescriptorTable(COMMON_MATERIAL_ROOT_TABLE_INDEX, materialHandle);
	}

	Optional<Material::BaseColor> Material::GetBaseColor() const
	{
		if (mType & MATERIAL_TYPE_BASECOLOR)
		{
			Material::BaseColor res = {};

			res.texture = mBaseColor;
			res.factor = XMFLOAT4(mBaseColorFactor.data());

			return std::move(res);
		}

		return std::nullopt;
	}

	Optional<Material::MetallicRoughness> Material::GetMetallicRoughness() const
	{
		if (mType & MATERIAL_TYPE_METALLIC_ROUGHNESS)
		{
			Material::MetallicRoughness res = {};

			res.texture = mMetallicRoughness;
			res.metallicFactor = mMetallicFactor;
			res.roughnessFactor = mRoughnessFactor;

			return std::move(res);
		}

		return std::nullopt;
	}

	Optional<Material::Normal> Material::GetNormal() const
	{
		if (mType & MATERIAL_TYPE_NORMAL)
		{
			Material::Normal res = {};

			res.texture = mNormal;
			res.scale = mNormalScale;

			return std::move(res);
		}

		return std::nullopt;
	}

	Optional<Material::Occlusion> Material::GetOcclusion() const
	{
		if (mType & MATERIAL_TYPE_OCCLUSION)
		{
			Material::Occlusion res = {};

			res.texture = mOcclusion;
			res.strength = mOcclusionStrength;

			return std::move(res);
		}

		return std::nullopt;
	}

	Optional<Material::Emissive> Material::GetEmissive() const
	{
		if (mType & MATERIAL_TYPE_EMISSIVE)
		{
			Material::Emissive res = {};

			res.texture = mEmissive;
			res.factor = XMFLOAT3(mEmissiveFactor.data());

			return std::move(res);
		}

		return std::nullopt;
	}
}