#include "pch.h"
#include "ZPrePass.h"
#include "ResourceManagers.h"
#include "FrameGraph.h"

namespace Amadeus
{
	ZPrePass::ZPrePass(SharedPtr<DeviceResources> device)
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
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(shaders.Get("ZPreVS.cso"));
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(shaders.Get("ZPrePS.cso"));
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 2;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		psoDesc.RTVFormats[1] = DXGI_FORMAT_R11G11B10_FLOAT;
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

	bool ZPrePass::PreCompute(SharedPtr<DeviceResources> device, ID3D12GraphicsCommandList* commandList)
	{
		return true;
	}

	void ZPrePass::PostPreCompute()
	{
	}

	void ZPrePass::Setup(FrameGraph& fg, FrameGraphBuilder& builder, FrameGraphNode* node)
	{
		mPosition = builder.Write(
			"ZPrePosition",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			fg, node);

		mNormal = builder.Write(
			"ZPreNormal",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R11G11B10_FLOAT,
			fg, node);

		mDepth = builder.Write(
			"ZPreDepth",
			FrameGraphResourceType::DEPTH,
			DXGI_FORMAT_D32_FLOAT,
			fg, node);
	}

	void ZPrePass::RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache)
	{
		mPosition->RegisterResource(device, descriptorCache);
		mNormal->RegisterResource(device, descriptorCache);
		mDepth->RegisterResource(device, descriptorCache);
	}

	bool ZPrePass::Execute(SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache)
	{
		FrameGraphPass::Execute(device, descriptorManager, descriptorCache);
		UINT curFrameIndex = device->GetCurrentFrameIndex();
		auto& commandList = mCommandLists[curFrameIndex];

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle = mDepth->GetWriteView(commandList.Get());
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle[] =
		{
			mPosition->GetWriteView(commandList.Get()),
			mNormal->GetWriteView(commandList.Get()),
		};

		commandList->ClearRenderTargetView(rtvHandle[0], BackgroundColor, 0, nullptr);
		commandList->ClearRenderTargetView(rtvHandle[1], BackgroundColor, 0, nullptr);
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		commandList->OMSetRenderTargets(2, &rtvHandle[0], FALSE, &dsvHandle);

		// Z Pre Render
		ZPreRender params = {};
		params.device = device;
		params.descriptorCache = descriptorCache;
		params.commandList = commandList.Get();

		Subject<ZPreRender>* zPreRender = Registry::instance().query<ZPreRender>("ZPreRender");
		if (zPreRender)
		{
			zPreRender->notify(params);
		}

		ThrowIfFailed(mCommandLists[curFrameIndex]->Close());
		ID3D12CommandList* ppCommandList[] = { mCommandLists[curFrameIndex].Get() };
		device->GetCommandQueue()->ExecuteCommandLists(1, ppCommandList);

		return true;
	}

	void ZPrePass::Destroy()
	{
		FrameGraphPass::Destroy();
	}
}