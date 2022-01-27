#include "pch.h"
#include "ShadowPass.h"
#include "ResourceManagers.h"
#include "FrameGraph.h"

namespace Amadeus
{
	ShadowPass::ShadowPass(SharedPtr<DeviceResources> device)
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
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(shaders.Get("ShadowMapVS.cso"));
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(shaders.Get("ShadowMapPS.cso"));
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
		psoDesc.RasterizerState.SlopeScaledDepthBias = -1.5f;
		psoDesc.RasterizerState.DepthBias = -100;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_D32_FLOAT;
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

	void ShadowPass::Setup(FrameGraph& fg, FrameGraphBuilder& builder, FrameGraphNode* node)
	{
		mShadowMap = builder.Write(
			"ShadowMap",
			FrameGraphResourceType::DEPTH,
			DXGI_FORMAT_D32_FLOAT,
			fg, node);
	}

	void ShadowPass::RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache)
	{
		mShadowMap->RegisterResource(device, descriptorCache, mWidth, mHeight);
	}

	bool ShadowPass::Execute(SharedPtr<DeviceResources> device, 
		SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache)
	{
		UINT curFrameIndex = device->GetCurrentFrameIndex();
		auto& commandList = mCommandLists[curFrameIndex];
		ThrowIfFailed(commandList->Reset(device->GetCommandAllocator(), mPipelineState.Get()));
		commandList->SetGraphicsRootSignature(mRootSignature.Get());

		ID3D12DescriptorHeap* ppHeaps[] = { descriptorCache->GetCbvSrvUavCache(device), descriptorManager->GetSamplerHeap() };
		mCommandLists[curFrameIndex]->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		const D3D12_VIEWPORT screenViewPort = { 0.0f, 0.0f, static_cast<FLOAT>(mWidth), static_cast<FLOAT>(mHeight), 0.0f, 1.0f };
		const D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(mWidth), static_cast<LONG>(mHeight) };
		commandList->RSSetViewports(1, &screenViewPort);
		commandList->RSSetScissorRects(1, &scissorRect);

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle = mShadowMap->GetWriteView(commandList.Get());
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

		// Shadow Map Render
		ShadowMapRender params = {};
		params.device = device;
		params.descriptorCache = descriptorCache;
		params.commandList = commandList.Get();

		Subject<ShadowMapRender>* shadowMapRender = Registry::instance().query<ShadowMapRender>("ShadowMapRender");
		if (shadowMapRender)
		{
			shadowMapRender->notify(params);
		}

		ThrowIfFailed(mCommandLists[curFrameIndex]->Close());
		ID3D12CommandList* ppCommandList[] = { mCommandLists[curFrameIndex].Get() };
		device->GetCommandQueue()->ExecuteCommandLists(1, ppCommandList);

		return true;
	}

	void ShadowPass::Destroy()
	{
		FrameGraphPass::Destroy();
	}

}