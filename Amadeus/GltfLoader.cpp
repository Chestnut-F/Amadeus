#include "pch.h"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_USE_CPP14
#include "tinygltf/tiny_gltf.h"
#include "ResourceManagers.h"
#include "GltfLoader.h"

namespace Amadeus
{
	void LoadSampler(tinygltf::Model& model, SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager)
	{
		for (const auto& sampler : model.samplers)
		{
			int magFilter = sampler.magFilter;
			int minFilter = sampler.minFilter;
			if (magFilter == -1)
				magFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
			if (minFilter == -1 || minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR)
				minFilter = TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
			if (minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
				minFilter = TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST;
			int warpS = sampler.wrapS;
			int warpT = sampler.wrapT;

			D3D12_SAMPLER_DESC samplerDesc = {};
			D3D12_FILTER& filter = samplerDesc.Filter;
			D3D12_TEXTURE_ADDRESS_MODE& addressU = samplerDesc.AddressU;
			D3D12_TEXTURE_ADDRESS_MODE& addressV = samplerDesc.AddressV;

			if (magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST
				&& minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST) {
				filter = D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT;
			}
			else if (magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST
				&& minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST) {
				filter = D3D12_FILTER::D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
			}
			else if (magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST
				&& minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR) {
				filter = D3D12_FILTER::D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
			}
			else if (magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST
				&& minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR) {
				filter = D3D12_FILTER::D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			}
			else if (magFilter == TINYGLTF_TEXTURE_FILTER_LINEAR
				&& minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST) {
				filter = D3D12_FILTER::D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
			}
			else if (magFilter == TINYGLTF_TEXTURE_FILTER_LINEAR
				&& minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST) {
				filter = D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			}
			else if (magFilter == TINYGLTF_TEXTURE_FILTER_LINEAR
				&& minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR) {
				filter = D3D12_FILTER::D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			}
			else if (magFilter == TINYGLTF_TEXTURE_FILTER_LINEAR
				&& minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR) {
				filter = D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			}
			else {
				throw Exception("Filter of sampler is invalid.");
			}

			
			if (warpS == TINYGLTF_TEXTURE_WRAP_REPEAT) {
				addressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			}
			else if (warpS == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE) {
				addressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			}
			else if (warpS == TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT) {
				addressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			}

			if (warpT == TINYGLTF_TEXTURE_WRAP_REPEAT) {
				addressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			}
			else if (warpT == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE) {
				addressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			}
			else if (warpT == TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT) {
				addressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			}

			samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.MinLOD = 0;
			samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
			samplerDesc.MipLODBias = 0.0f;
			samplerDesc.MaxAnisotropy = 1;
			samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

			descriptorManager->AllocateSamplerHeap(device, samplerDesc);
		}
	}

	void LoadTexture(WString&& fileName, tinygltf::Model& model, SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager)
	{
		TextureManager& textureManager = TextureManager::Instance();
		assert(textureManager.Empty());

		for (auto& tex : model.textures)
		{
			auto& image = model.images[tex.source];

			textureManager.LoadFromFile(
				WString(L"Models\\" + fileName + L"\\" + String2WString(image.uri)), device, descriptorManager);
		}
	}

	void SetMaterialPBRMetallicRoughness(Material* dstMaterial, const tinygltf::Material& srcMat)
	{
		const auto& pbrMetallicRoughness = srcMat.pbrMetallicRoughness;
		const auto& baseColorTexture = pbrMetallicRoughness.baseColorTexture;
		if (baseColorTexture.texCoord != 0)
		{
			throw Exception("Base color texture coordinate is invalid.");	// 暂时不支持texCoord_N
		}

		const auto& metallicRoughnessTexture = pbrMetallicRoughness.metallicRoughnessTexture;
		if (metallicRoughnessTexture.texCoord != 0)
		{
			throw Exception("Metallic roughness texture coordinate is invalid.");	// 暂时不支持texCoord_N
		}

		Vector<float> baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
		std::copy(pbrMetallicRoughness.baseColorFactor.begin(), pbrMetallicRoughness.baseColorFactor.end(), baseColorFactor.begin());

		dstMaterial->SetPBRMetallicRoughness(
			std::move(baseColorFactor),
			pbrMetallicRoughness.metallicFactor,
			pbrMetallicRoughness.roughnessFactor);
	}

	void SetMaterialNormal(Material* dstMaterial, const tinygltf::Material& srcMat)
	{
		const auto& normalTexture = srcMat.normalTexture;
		if (normalTexture.texCoord != 0)
		{
			throw Exception("normal texture coordinate is invalid.");	// 暂时不支持texcoord_n
		}

		dstMaterial->SetNormal(normalTexture.scale);
	}

	void SetMaterialOcclusion(Material* dstMaterial, const tinygltf::Material& srcMat)
	{
		auto& occlusionTexture = srcMat.occlusionTexture;
		if (occlusionTexture.texCoord != 0)
		{
			throw Exception("Occlusion texture coordinate is invalid.");	// 暂时不支持texCoord_N
		}

		dstMaterial->SetOcclusion(occlusionTexture.strength);
	}

	void SetMaterialEmissive(Material* dstMaterial, const tinygltf::Material& srcMat)
	{
		const auto& emissiveTexture = srcMat.emissiveTexture;
		if (emissiveTexture.texCoord != 0)
		{
			throw Exception("Emissive texture coordinate is invalid.");	// 暂时不支持texCoord_N
		}

		Vector<float> emissiveFactor = { 0.0f, 0.0f, 0.0f };
		std::copy(srcMat.emissiveFactor.begin(), srcMat.emissiveFactor.end(), emissiveFactor.begin());

		dstMaterial->SetEmissive(std::move(emissiveFactor));
	}

	void LoadMaterial(tinygltf::Model& model)
	{
		for (auto& mat : model.materials)
		{
			Material::TextureId baseColorId			= mat.pbrMetallicRoughness.baseColorTexture.index;
			Material::TextureId metallicRoughnessId = mat.pbrMetallicRoughness.metallicRoughnessTexture.index;
			Material::TextureId normalId			= mat.normalTexture.index;
			Material::TextureId occlusionId			= mat.occlusionTexture.index;
			Material::TextureId emissiveId			= mat.emissiveTexture.index;
			UINT64 index = MaterialManager::Instance().CreateMaterial(
				baseColorId, metallicRoughnessId, normalId, occlusionId, emissiveId);
			Material* pMaterial = MaterialManager::Instance().GetMaterial(index);

			SetMaterialPBRMetallicRoughness(pMaterial, mat);

			SetMaterialNormal(pMaterial, mat);

			SetMaterialOcclusion(pMaterial, mat);

			SetMaterialEmissive(pMaterial, mat);

			float alphaCutoff;
			Material::MATERIAL_ALPHA_MODE alphaMode;
			if (mat.alphaMode == "OPAQUE")
			{
				alphaMode = Material::MATERIAL_ALPHA_MODE::MATERIAL_OPAQUE;
			}
			else if (mat.alphaMode == "MASK")
			{
				alphaMode = Material::MATERIAL_ALPHA_MODE::MATERIAL_MASK;
			}
			else if (mat.alphaMode == "BLEND")
			{
				alphaMode = Material::MATERIAL_ALPHA_MODE::MATERIAL_BLEND;
			}
			alphaCutoff = mat.alphaCutoff;
			pMaterial->SetAlphaMode(alphaCutoff, alphaMode);

			pMaterial->SetDoubleSided(mat.doubleSided);
		}
	}

	// Validate that an accessor does not go out of bounds of the buffer view that it references and that the buffer view does not exceed
	// the bounds of the buffer that it references.
	void ValidateAccessor(const tinygltf::Accessor& accessor, const tinygltf::BufferView& bufferView,
		const Vector<uint8_t>& buffer, size_t byteStride, size_t elementSize)
	{
		// Make sure the accessor does not go out of range of the buffer view.
		if (accessor.byteOffset + (accessor.count - 1) * byteStride + elementSize > bufferView.byteLength)
		{
			throw OutOfRange("Accessor goes out of range of bufferview.");
		}

		// Make sure the buffer view does not go out of range of the buffer.
		if (bufferView.byteOffset + bufferView.byteLength > buffer.size())
		{
			throw OutOfRange("BufferView goes out of range of buffer.");
		}
	}

	void CreatePrimitivePositionDesc(const tinygltf::Model& model,
		const tinygltf::Accessor& accessor, Vector<Primitive::Vertex>& vertices)
	{
		const auto& bufferView = model.bufferViews[accessor.bufferView];
		if (bufferView.target != TINYGLTF_TARGET_ARRAY_BUFFER && bufferView.target != 0)
		{
			throw Exception("Primitive position info(Buffer View Target) is invalid.");
		}

		const auto& buffer = model.buffers[bufferView.buffer].data;
		constexpr size_t PackedSize = sizeof(XMFLOAT3);
		const size_t stride = bufferView.byteStride == 0 ? PackedSize : bufferView.byteStride;
		ValidateAccessor(accessor, bufferView, buffer, stride, PackedSize);

		const uint8_t* bufferPtr = buffer.data() + bufferView.byteOffset + accessor.byteOffset;
		for (uint32_t i = 0; i < accessor.count; ++i, bufferPtr += stride)
		{
			vertices[i].position = *reinterpret_cast<const XMFLOAT3*>(bufferPtr);
		}
	}

	void CreatePrimitiveNormalDesc(const tinygltf::Model& model,
		const tinygltf::Accessor& accessor, Vector<Primitive::Vertex>& vertices)
	{
		const auto& bufferView = model.bufferViews[accessor.bufferView];
		if (bufferView.target != TINYGLTF_TARGET_ARRAY_BUFFER && bufferView.target != 0)
		{
			throw Exception("Primitive normal info(Buffer View Target) is invalid.");
		}

		const auto& buffer = model.buffers[bufferView.buffer].data;
		constexpr size_t PackedSize = sizeof(XMFLOAT3);
		const size_t stride = bufferView.byteStride == 0 ? PackedSize : bufferView.byteStride;
		ValidateAccessor(accessor, bufferView, buffer, stride, PackedSize);

		const uint8_t* bufferPtr = buffer.data() + bufferView.byteOffset + accessor.byteOffset;
		for (uint32_t i = 0; i < accessor.count; ++i, bufferPtr += stride)
		{
			vertices[i].normal = *reinterpret_cast<const XMFLOAT3*>(bufferPtr);
		}
	}

	void CreatePrimitiveTangentDesc(const tinygltf::Model& model,
		const tinygltf::Accessor& accessor, Vector<Primitive::Vertex>& vertices)
	{
		const auto& bufferView = model.bufferViews[accessor.bufferView];
		if (bufferView.target != TINYGLTF_TARGET_ARRAY_BUFFER && bufferView.target != 0)
		{
			throw Exception("Primitive tangent info(Buffer View Target) is invalid.");
		}

		const auto& buffer = model.buffers[bufferView.buffer].data;
		constexpr size_t PackedSize = sizeof(XMFLOAT4);
		const size_t stride = bufferView.byteStride == 0 ? PackedSize : bufferView.byteStride;
		ValidateAccessor(accessor, bufferView, buffer, stride, PackedSize);

		const uint8_t* bufferPtr = buffer.data() + bufferView.byteOffset + accessor.byteOffset;
		for (uint32_t i = 0; i < accessor.count; ++i, bufferPtr += stride)
		{
			vertices[i].tangent = *reinterpret_cast<const XMFLOAT4*>(bufferPtr);
		}
	}

	template<typename T>
	void CreatePrimitiveTexCoordDesc(const tinygltf::Model& model,
		const tinygltf::Accessor& accessor, Vector<Primitive::Vertex>& vertices)
	{
		const auto& bufferView = model.bufferViews[accessor.bufferView];
		if (bufferView.target != TINYGLTF_TARGET_ARRAY_BUFFER && bufferView.target != 0)
		{
			throw Exception("Primitive texture coordinate 0 info(Buffer View Target) is invalid.");
		}

		const auto& buffer = model.buffers[bufferView.buffer].data;
		constexpr size_t PackedSize = 2 * sizeof(T);
		const size_t stride = bufferView.byteStride == 0 ? PackedSize : bufferView.byteStride;
		ValidateAccessor(accessor, bufferView, buffer, stride, PackedSize);

		const uint8_t* bufferPtr = buffer.data() + bufferView.byteOffset + accessor.byteOffset;
		for (uint32_t i = 0; i < accessor.count; ++i, bufferPtr += stride)
		{
			vertices[i].texCoord0.x = *reinterpret_cast<const T*>(bufferPtr) / (float)(NumericLimits<T>::max)();
			vertices[i].texCoord0.y = *reinterpret_cast<const T*>(bufferPtr + sizeof(T)) / (float)(NumericLimits<T>::max)();
		}
	}

	template<>
	void CreatePrimitiveTexCoordDesc<float>(const tinygltf::Model& model,
		const tinygltf::Accessor& accessor, Vector<Primitive::Vertex>& vertices)
	{
		const auto& bufferView = model.bufferViews[accessor.bufferView];
		if (bufferView.target != TINYGLTF_TARGET_ARRAY_BUFFER && bufferView.target != 0)
		{
			throw Exception("Primitive texture coordinate 0 info(Buffer View Target) is invalid.");
		}

		const auto& buffer = model.buffers[bufferView.buffer].data;
		constexpr size_t PackedSize = 2 * sizeof(float);
		const size_t stride = bufferView.byteStride == 0 ? PackedSize : bufferView.byteStride;
		ValidateAccessor(accessor, bufferView, buffer, stride, PackedSize);

		const uint8_t* bufferPtr = buffer.data() + bufferView.byteOffset + accessor.byteOffset;
		for (uint32_t i = 0; i < accessor.count; ++i, bufferPtr += stride)
		{
			vertices[i].texCoord0.x = *reinterpret_cast<const float*>(bufferPtr);
			vertices[i].texCoord0.y = *reinterpret_cast<const float*>(bufferPtr + sizeof(float));
		}
	}

	void CreatePrimitiveVerticesDesc(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
		Vector<Primitive::Vertex>& vertices, bool& hasNormals, bool& hasTangents)
	{
		/* Create position of vertices. */
		if (primitive.attributes.find("POSITION") != primitive.attributes.end())
		{
			const auto& accessor = model.accessors[primitive.attributes.find("POSITION")->second];

			if (accessor.type != TINYGLTF_TYPE_VEC3)
			{
				throw Exception("Primitive position info (Accessor Type) is invalid (VEC3 expected).");
			}
			if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
			{
				throw Exception("Primitive position info (Accessor Component Type) is invalid (FLOAT expected).");
			}
			if (accessor.bufferView == -1)
			{
				throw Exception("Primitive position info (Buffer View) is invalid.");
			}

			// 创建Postion时确定vertices所需空间，初始化所需空间
			vertices.resize(accessor.count);

			CreatePrimitivePositionDesc(model, accessor, vertices);
		}
		else
		{
			throw Exception("Primitive position info (Accessor) is invalid.");
		}

		/* Create normal of vertices. */
		if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
		{
			const auto& accessor = model.accessors[primitive.attributes.find("NORMAL")->second];

			if (accessor.type != TINYGLTF_TYPE_VEC3)
			{
				throw Exception("Primitive normal info (Accessor Type) is invalid (VEC3 expected).");
			}
			if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
			{
				throw Exception("Primitive normal info (Accessor Component Type) is invalid (FLOAT expected).");
			}
			if (accessor.bufferView == -1)
			{
				throw Exception("Primitive normal info (Buffer View) is invalid.");
			}

			CreatePrimitiveNormalDesc(model, accessor, vertices);

			hasNormals = true;
		}
		else
		{
			hasNormals = false;
		}

		/* Create tangent of vertices. */
		if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
		{
			const auto& accessor = model.accessors[primitive.attributes.find("TANGENT")->second];

			if (accessor.type != TINYGLTF_TYPE_VEC4)
			{
				throw Exception("Primitive tangent info (Accessor Type) is invalid (VEC4 expected).");
			}
			if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
			{
				throw Exception("Primitive tangent info (Accessor Component Type) is invalid (FLOAT expected).");
			}
			if (accessor.bufferView == -1)
			{
				throw Exception("Primitive tangent info (Buffer View) is invalid.");
			}

			CreatePrimitiveTangentDesc(model, accessor, vertices);

			hasTangents = true;
		}
		else
		{
			hasTangents = false;
		}

		/* Create texCoord0 of vertices. */
		if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
		{
			//throw Exception("Primitive texture coordinate 0 info (Accessor) is invalid.");
			const auto& accessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
			if (accessor.type != TINYGLTF_TYPE_VEC2)
			{
				throw Exception("Primitive texture coordinate 0 info (Accessor Type) is invalid (VEC2 expected).");
			}
			if (accessor.bufferView == -1)
			{
				throw Exception("Primitive texture coordinate 0 info (Buffer View) is invalid.");
			}
			if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
			{
				if (!accessor.normalized)
				{
					throw Exception("Accessor for TEXCOORD_n unsigned byte must be normalized.");
				}

				CreatePrimitiveTexCoordDesc<uint8_t>(model, accessor, vertices);
			}
			else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
			{
				if (!accessor.normalized)
				{
					throw Exception("Accessor for TEXCOORD_n unsigned short must be normalized.");
				}

				CreatePrimitiveTexCoordDesc<uint8_t>(model, accessor, vertices);
			}
			else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
			{
				CreatePrimitiveTexCoordDesc<float>(model, accessor, vertices);
			}
			else
			{
				throw Exception("Primitive normal info (Accessor Component Type) is invalid.");
			}
		}
	}

	template<typename T>
	void CreatePrimitiveIndicesDesc(const tinygltf::Model& model,
		const tinygltf::Accessor& accessor, Vector<uint32_t>& indices)
	{
		const auto& bufferView = model.bufferViews[accessor.bufferView];
		if (bufferView.target != TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER && bufferView.target != 0)
		{
			throw Exception("Primitive indices info(Buffer View Target) is invalid.");
		}
		if (bufferView.byteStride != 0 && bufferView.byteStride != sizeof(T))
		{
			throw Exception("Primitive indices info(Buffer View Byte Stride) is invalid.");
		}

		const auto& buffer = model.buffers[bufferView.buffer].data;
		constexpr size_t PackedSize = sizeof(T);
		const size_t stride = bufferView.byteStride == 0 ? PackedSize : bufferView.byteStride;
		ValidateAccessor(accessor, bufferView, buffer, stride, PackedSize);

		const uint8_t* bufferPtr = buffer.data() + bufferView.byteOffset + accessor.byteOffset;
		indices.reserve(accessor.count);
		for (uint32_t i = 0; i < accessor.count; ++i, bufferPtr += stride)
		{
			indices.emplace_back(*reinterpret_cast<const T*>(bufferPtr));
		}
	}

	void CreatePrimitiveIndicesDesc(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
		const Vector<Primitive::Vertex>& vertices, Vector<uint32_t>& indices)
	{
		if (primitive.indices != -1)
		{
			auto& accessor = model.accessors[primitive.indices];
			if (accessor.type != TINYGLTF_TYPE_SCALAR)
			{
				throw Exception("Primitive indices info (Accessor Type) is invalid (SCALAR expected).");
			}
			if (accessor.bufferView == -1)
			{
				throw Exception("Primitive indices info (Buffer View) is invalid.");
			}
			// Since only triangles are supported, enforce that the number of indices is divisible by 3.
			if ((accessor.count % 3) != 0)
			{
				throw Exception("Unexpected number of indices for triangle primitive.");
			}

			if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
			{
				CreatePrimitiveIndicesDesc<uint8_t>(model, accessor, indices);
			}
			else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
			{
				CreatePrimitiveIndicesDesc<uint16_t>(model, accessor, indices);
			}
			else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
			{
				CreatePrimitiveIndicesDesc<uint32_t>(model, accessor, indices);
			}
			else
			{
				throw Exception("Primitive indices info (Accessor Component Type) is invalid.");
			}
		}
		else
		{
			// When indices is not defined, the primitives should be rendered without indices using drawArrays()
			// This is the equivalent to having an index in sequence for each vertex.
			const uint32_t vertexCount = (uint32_t)vertices.size();
			if ((vertexCount % 3) != 0)
			{
				throw Exception("Non-indexed triangle-based primitive must have number of vertices divisible by 3.");
			}

			indices.reserve(vertexCount);
			for (uint32_t i = 0; i < vertexCount; i++)
			{
				indices.emplace_back(i);
			}
		}
	}

	XMMATRIX CreateNodeTransform(const tinygltf::Node& node)
	{
		if (node.matrix.size() > 0)
		{
			Vector<float> matrix;
			for (uint32_t i = 0; i != node.matrix.size(); ++i)
			{
				matrix.emplace_back(static_cast<float>(node.matrix[i]));
			}
			return XMMATRIX(matrix.data());
		}
		else
		{
			XMMATRIX translation = XMMatrixIdentity();
			XMMATRIX rotation = XMMatrixIdentity();
			XMMATRIX scale = XMMatrixIdentity();

			if (node.translation.size() > 0)
			{
				translation = XMMATRIX(
					1.0f, 0.0f, 0.0f, node.translation[0],
					0.0f, 1.0f, 0.0f, node.translation[1],
					0.0f, 0.0f, 1.0f, node.translation[2],
					0.0f, 0.0f, 0.0f, 0.0f
				);
			}

			if (node.rotation.size() > 0)
			{
				XMVECTOR quaternion = { node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3] };
				rotation = XMMatrixRotationQuaternion(quaternion);
			}

			if (node.scale.size() > 0)
			{
				scale = XMMATRIX(
					node.scale[0], 0.0f, 0.0f, 0.0f,
					0.0f, node.scale[1], 0.0f, 0.0f,
					0.0f, 0.0f, node.scale[2], 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f
				);
			}

			return XMMatrixMultiply(XMMatrixMultiply(translation, rotation), scale);
		}
	}

	void LoadMesh(tinygltf::Model& model, SharedPtr<DeviceResources> device)
	{
		MeshManager& meshManager = MeshManager::Instance();

		for (auto& node : model.nodes)
		{
			if (node.mesh > -1)
			{
				auto& mesh = model.meshes[node.mesh];

				UINT64 meshId = MeshManager::Instance().CreateMesh(CreateNodeTransform(node));
				Mesh* pMesh = MeshManager::Instance().GetMesh(meshId);

				for (auto& primitive : mesh.primitives)
				{
					if (primitive.mode != TINYGLTF_MODE_TRIANGLES)
					{
						throw Exception("Primitive topology mode is invalid.");
					}
					INT material = primitive.material;
					bool hasNormals = false;
					bool hasTangents = false;

					Vector<Primitive::Vertex> vertices;
					Vector<UINT> indices;

					CreatePrimitiveVerticesDesc(model, primitive, vertices, hasNormals, hasTangents);

					CreatePrimitiveIndicesDesc(model, primitive, vertices, indices);

					pMesh->CreatrPrimitive(std::move(vertices), std::move(indices), material, hasNormals, hasTangents);
				}
			}
		}
	}

	void Gltf::LoadGltf(WString&& fileName, SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager)
	{
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		String err;
		String warn;

		String fullPath = WString2String(GetAssetFullPath(L"Assets\\Models\\" + fileName + L"\\" + fileName + L".gltf"));
		bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, fullPath);

		if (!err.empty()) {
			throw(RuntimeError(err));
		}

		LoadSampler(model, device, descriptorManager);
		LoadTexture(std::move(fileName), model, device, descriptorManager);
		LoadMaterial(model);
		LoadMesh(model, device);
	}
}