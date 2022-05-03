#include "pch.h"
#include "SkyboxPass.h"
#include "ResourceManagers.h"
#include "FrameGraph.h"

namespace Amadeus
{
	SkyboxPass::SkyboxPass(SharedPtr<DeviceResources> device)
		: FrameGraphPass::FrameGraphPass(false)
	{
		ProgramManager& shaders = ProgramManager::Instance();

		ThrowIfFailed(device->GetD3DDevice()->CreateRootSignature(
			0,
			shaders.Get("SkyboxRS.cso")->GetBufferPointer(),
			shaders.GetBufferSize("SkyboxRS.cso"),
			IID_PPV_ARGS(&mRootSignature)
		));

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { nullptr, 0 };
		psoDesc.pRootSignature = mRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(shaders.Get("SkyboxVS.cso"));
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(shaders.Get("SkyboxPS.cso"));
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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

	bool SkyboxPass::PreCompute(SharedPtr<DeviceResources> device, ID3D12GraphicsCommandList* commandList)
	{
		return true;
	}

	void SkyboxPass::PostPreCompute()
	{
	}

	void SkyboxPass::Setup(FrameGraph& fg, FrameGraphBuilder& builder, FrameGraphNode* node)
	{
		mBaseColor = builder.Read(
			"BaseColor",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			fg, node);

		mDepth = builder.Read(
			"ZPreDepth",
			FrameGraphResourceType::DEPTH,
			DXGI_FORMAT_D32_FLOAT,
			fg, node);

		mSky = builder.Write(
			"Sky",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			fg, node);
	}

	void SkyboxPass::RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache)
	{
		mSky->RegisterResource(device, descriptorCache);
	}

	bool SkyboxPass::Execute(SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache)
	{
		FrameGraphPass::Execute(device, descriptorManager, descriptorCache);
		UINT curFrameIndex = device->GetCurrentFrameIndex();
		auto& commandList = mCommandLists[curFrameIndex];

		const CD3DX12_RESOURCE_BARRIER Present2RenderTarget =
			CD3DX12_RESOURCE_BARRIER::Transition(
				device->GetRenderTarget(),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ResourceBarrier(1, &Present2RenderTarget);

		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView = mBaseColor->GetWriteView(commandList.Get());
		D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = mDepth->GetDepthStencilView(commandList.Get());;
		commandList->OMSetRenderTargets(1, &renderTargetView, false, &depthStencilView);

		Texture* skybox = TextureManager::Instance().GetTexture(EngineVar::CUBEMAP_ENNIS_ID);
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle = descriptorCache->AppendSrvCache(device, skybox->GetDescriptorHandle());
		commandList->SetGraphicsRootDescriptorTable(1, srvHandle);

		// Skybox Render
		SkyboxRender params = {};
		params.device = device;
		params.descriptorCache = descriptorCache;
		params.commandList = commandList.Get();

		Subject<SkyboxRender>* skyboxRender = Registry::instance().query<SkyboxRender>("SkyboxRender");
		if (skyboxRender)
		{
			skyboxRender->notify(params);
		}

		commandList->IASetVertexBuffers(0, 0, nullptr);
		commandList->IASetIndexBuffer(nullptr);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->DrawInstanced(36, 1, 0, 0);

		const CD3DX12_RESOURCE_BARRIER RenderTarget2Present =
			CD3DX12_RESOURCE_BARRIER::Transition(
				device->GetRenderTarget(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT);
		commandList->ResourceBarrier(1, &RenderTarget2Present);

		ThrowIfFailed(commandList->Close());

		ID3D12CommandList* ppCommandList[] = { commandList.Get() };
		device->GetCommandQueue()->ExecuteCommandLists(1, ppCommandList);

		return true;
	}

	void SkyboxPass::Destroy()
	{
		FrameGraphPass::Destroy();
	}
}
