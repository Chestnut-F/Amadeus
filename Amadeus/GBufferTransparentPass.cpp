#include "pch.h"
#include "GBufferTransparentPass.h"
#include "RenderSystem.h"
#include "ResourceManagers.h"
#include "FrameGraph.h"

namespace Amadeus
{
	GBufferTransparentPass::GBufferTransparentPass(SharedPtr<DeviceResources> device)
		: FrameGraphPass::FrameGraphPass(false)
	{
		ProgramManager& shaders = ProgramManager::Instance();

		ThrowIfFailed(device->GetD3DDevice()->CreateRootSignature(
			0,
			shaders.Get("Common.cso")->GetBufferPointer(),
			shaders.GetBufferSize("Common.cso"),
			IID_PPV_ARGS(&mRootSignature)
		));

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = mRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(shaders.Get("Model.cso"));
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(shaders.Get("GBufferTransparent.cso"));
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 4;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R10G10B10A2_UNORM;
		psoDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoDesc.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.RTVFormats[3] = DXGI_FORMAT_R16G16_FLOAT;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(device->GetD3DDevice()->CreateGraphicsPipelineState(
			&psoDesc,
			IID_PPV_ARGS(&mPipelineState)));

		for (UINT i = 0; i < FrameCount; ++i)
		{
			ComPtr<ID3D12GraphicsCommandList> commandList;

			ThrowIfFailed(device->GetD3DDevice()->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				device->GetCommandAllocator(),
				mPipelineState.Get(),
				IID_PPV_ARGS(&commandList)));

			mCommandLists.emplace_back(commandList);

			ThrowIfFailed(commandList->Close());
		}
	}

	bool GBufferTransparentPass::PreCompute(SharedPtr<DeviceResources> device, ID3D12GraphicsCommandList* commandList)
	{
		return true;
	}

	void GBufferTransparentPass::PostPreCompute()
	{
	}

	void GBufferTransparentPass::Setup(FrameGraph& fg, FrameGraphBuilder& builder, FrameGraphNode* node)
	{
		mTransparent = builder.Write(
			"Transparent",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			fg, node);

		mNormal = builder.Read(
			"Normal",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R10G10B10A2_UNORM,
			fg, node);

		mBaseColor = builder.Read(
			"BaseColor",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			fg, node);

		mMetallicSpecularRoughness = builder.Read(
			"MetallicSpecularRoughness",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			fg, node);

		mVelocity = builder.Read(
			"Velocity",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R16G16_FLOAT,
			fg, node);

		mDepth = builder.Read(
			"ZPreDepth",
			FrameGraphResourceType::DEPTH,
			DXGI_FORMAT_D32_FLOAT,
			fg, node);
	}

	void GBufferTransparentPass::RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache)
	{
		mTransparent->RegisterResource(device, descriptorCache);
	}

	bool GBufferTransparentPass::Execute(SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache)
	{
		FrameGraphPass::Execute(device, descriptorManager, descriptorCache);
		UINT curFrameIndex = device->GetCurrentFrameIndex();
		auto& commandList = mCommandLists[curFrameIndex];

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle = mDepth->GetDepthStencilView(commandList.Get());
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle[] =
		{
			mNormal->GetWriteView(commandList.Get()),
			mBaseColor->GetWriteView(commandList.Get()),
			mMetallicSpecularRoughness->GetWriteView(commandList.Get()),
			mVelocity->GetWriteView(commandList.Get())
		};

		commandList->OMSetRenderTargets(4, &rtvHandle[0], FALSE, &dsvHandle);

		commandList->SetGraphicsRootDescriptorTable(
			COMMON_SAMPLER_ROOT_TABLE_INDEX, descriptorManager->GetSamplerHeap()->GetGPUDescriptorHandleForHeapStart());

		Texture* skybox = TextureManager::Instance().GetTexture(EngineVar::CUBEMAP_ENNIS_ID);
		Texture* lut = TextureManager::Instance().GetTexture(EngineVar::TEXTURE_BRDF_LUT_ID);
		CD3DX12_GPU_DESCRIPTOR_HANDLE iblHandle[] = {
			descriptorCache->AppendSrvCache(device, lut->GetDescriptorHandle()),
			descriptorCache->AppendSrvCache(device, skybox->GetDescriptorHandle()),
		};
		commandList->SetGraphicsRootDescriptorTable(7, *iblHandle);

		// GBuffer Transparent Render
		GBufferTransparentRender params = {};
		params.device = device;
		params.descriptorCache = descriptorCache;
		params.commandList = commandList.Get();

		Subject<GBufferTransparentRender>* gBufferTransparentRender = Registry::instance().query<GBufferTransparentRender>("GBufferTransparentRender");
		if (gBufferTransparentRender)
		{
			gBufferTransparentRender->notify(params);
		}

		ThrowIfFailed(mCommandLists[curFrameIndex]->Close());
		ID3D12CommandList* ppCommandList[] = { mCommandLists[curFrameIndex].Get() };
		device->GetCommandQueue()->ExecuteCommandLists(1, ppCommandList);

		return true;
	}

	void GBufferTransparentPass::Destroy()
	{
		FrameGraphPass::Destroy();
	}
}