#pragma once
#include "Prerequisites.h"

namespace Amadeus
{
	static constexpr UINT32 MATERIAL_TYPE_NONE					= 0x00;
	static constexpr UINT32 MATERIAL_TYPE_BASECOLOR				= 0x01;
	static constexpr UINT32 MATERIAL_TYPE_METALLIC_ROUGHNESS	= 0x02;
	static constexpr UINT32 MATERIAL_TYPE_NORMAL				= 0x04;
	static constexpr UINT32 MATERIAL_TYPE_OCCLUSION				= 0x08;
	static constexpr UINT32 MATERIAL_TYPE_EMISSIVE				= 0x10;

	class Texture;

	class Material
	{
	public:
		struct MaterialConstantBuffer
		{
			XMFLOAT4 baseColorFactor;
			float metallicFactor;
			float roughnessFactor;
			float normalScale;
			float occlusionStrength;
			XMFLOAT3 emissiveFactor;
			UINT32 materialType;
			float padding[52];
		};
		static_assert((sizeof(MaterialConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

		enum class MATERIAL_ALPHA_MODE : UINT8
		{
			MATERIAL_OPAQUE = 1,
			MATERIAL_MASK = 2,
			MATERIAL_BLEND = 3
		};

		typedef INT TextureId;
		typedef INT TexCoord;

	public:
		explicit Material(TextureId baseColorId, TextureId metallicRoughnessId,
			TextureId normalId, TextureId occlusionId, TextureId emissiveId);
		Material(const Material&) = delete;
		Material& operator=(const Material&) = delete;
		~Material() = default;

		void SetPBRMetallicRoughness(Vector<float>&& baseColorFactor = { 1.0f ,1.0f, 1.0f, 1.0f },
			float metallicFactor = 1.0f, float roughnessFactor = 1.0f);

		void SetNormal(float normalScale);

		void SetOcclusion(float occlusionStrength);

		void SetEmissive(Vector<float>&& emissiveFactor);

		void SetAlphaMode(float alphaCutoff, MATERIAL_ALPHA_MODE alphaMode);

		void SetDoubleSided(bool bDouble);

		bool Upload(SharedPtr<DeviceResources> device);

		void Render(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache, ID3D12GraphicsCommandList* commandList);

		void Destroy();

		struct BaseColor
		{
			Texture* texture;
			XMFLOAT4 factor;
		};
		Optional<BaseColor> GetBaseColor() const;

		struct MetallicRoughness
		{
			Texture* texture;
			float metallicFactor;
			float roughnessFactor;
		};
		Optional<MetallicRoughness> GetMetallicRoughness() const;

		struct Normal
		{
			Texture* texture;
			float scale;
		};
		Optional<Normal> GetNormal() const;

		struct Occlusion
		{
			Texture* texture;
			float strength;
		};
		Optional<Occlusion> GetOcclusion() const;

		struct Emissive
		{
			Texture* texture;
			XMFLOAT3 factor;
		};
		Optional<Emissive> GetEmissive() const;

		UINT32 Type() { return mType; }

		ID3D12Resource* GetD3D12Resource() { return mMaterialConstants.Get(); }

	private:
		// Type Flag
		UINT32 mType = 0;

		// PBR Metallic Roughness
		Texture* mBaseColor;
		TextureId mBaseColorId = -1;
		TexCoord mBaseColorCoord = 0; // Note: This implementation does not currently support glTF 2's TexCoord1 attributes.
		Vector<float> mBaseColorFactor = { 1.0f ,1.0f, 1.0f, 1.0f };

		Texture* mMetallicRoughness;
		TextureId mMetallicRoughnessId = -1;
		TexCoord mMetallicRoughnessCoord = 0; // Note: This implementation does not currently support glTF 2's TexCoord1 attributes.
		float mMetallicFactor = 1.0f;
		float mRoughnessFactor = 1.0f;

		// Normal
		Texture* mNormal;
		TextureId mNormalId = -1;
		TexCoord mNormalCoord = 0; // Note: This implementation does not currently support glTF 2's TexCoord1 attributes.
		float mNormalScale = 1.0f;

		// Occlusion
		Texture* mOcclusion;
		TextureId mOcclusionId = -1;
		TexCoord mOcclusionCoord = 0; // Note: This implementation does not currently support glTF 2's TexCoord1 attributes.
		float mOcclusionStrength = 1.0f;

		// Emissive
		Texture* mEmissive;
		TextureId mEmissiveId = -1;
		TexCoord mEmissiveCoord = 0; // Note: This implementation does not currently support glTF 2's TexCoord1 attributes.
		Vector<float> mEmissiveFactor = { 0.0f, 0.0f, 0.0f };

		// Alpha Mode
		float mAlphaCutoff = 0.5f;
		MATERIAL_ALPHA_MODE mAlphaMode = MATERIAL_ALPHA_MODE::MATERIAL_OPAQUE;

		// Other
		bool bDoubleSided = false;

		// Flag
		UINT32 bInitialized = 0;
		UINT32 bUploaded = 0;

		// D3D12 Resource
		ComPtr<ID3D12Resource> mMaterialConstants;
		MaterialConstantBuffer mMaterialConstantBuffer;
		UINT8* pMaterialCbvDataBegin;
		const UINT mMaterialConstantBufferSize = sizeof(MaterialConstantBuffer);
	};
}