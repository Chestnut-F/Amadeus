#include "pch.h"
#include <D3Dcompiler.h>
#include "GBufferPass.h"
#include "RenderSystem.h"
#include "ResourceManagers.h"
#include "FrameGraph.h"

namespace Amadeus
{
    GBufferPass::GBufferPass(SharedPtr<DeviceResources> device)
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
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(shaders.Get("GBuffer.cso"));
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 4;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R10G10B10A2_UNORM;
		psoDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoDesc.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.RTVFormats[3] = DXGI_FORMAT_R8G8B8A8_UNORM;
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

	void GBufferPass::Setup(FrameGraph& fg, FrameGraphBuilder& builder, FrameGraphNode* node)
	{
		mNormal = builder.Write(
			"Normal",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R10G10B10A2_UNORM,
			fg, node);

		mBaseColor = builder.Write(
			"BaseColor",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			fg, node);

		mMetallicSpecularRoughness = builder.Write(
			"MetallicSpecularRoughness",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			fg, node);

		mVelocity = builder.Write(
			"Velocity",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			fg, node);

		mDepth = builder.Write(
			"Depth",
			FrameGraphResourceType::DEPTH,
			DXGI_FORMAT_D32_FLOAT,
			fg, node);
	}

	void GBufferPass::RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache)
	{
		mNormal->RegisterResource(device, descriptorCache);
		mBaseColor->RegisterResource(device, descriptorCache);
		mMetallicSpecularRoughness->RegisterResource(device, descriptorCache);
		mVelocity->RegisterResource(device, descriptorCache);
		mDepth->RegisterResource(device, descriptorCache);
	}

	bool GBufferPass::Execute(SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache)
	{
		UINT curFrameIndex = device->GetCurrentFrameIndex();
		auto& commandList = mCommandLists[curFrameIndex];
		ThrowIfFailed(commandList->Reset(device->GetCommandAllocator(), mPipelineState.Get()));
		commandList->SetGraphicsRootSignature(mRootSignature.Get());

		ID3D12DescriptorHeap* ppHeaps[] = { descriptorCache->GetCbvSrvUavCache(device), descriptorManager->GetSamplerHeap() };
		mCommandLists[curFrameIndex]->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		const D3D12_VIEWPORT screenViewPort = device->GetScreenViewport();
		const D3D12_RECT scissorRect = device->GetScissorRect();
		commandList->RSSetViewports(1, &screenViewPort);
		commandList->RSSetScissorRects(1, &scissorRect);

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle = mDepth->GetWriteView(commandList.Get());
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle[] =
		{
			mNormal->GetWriteView(commandList.Get()),
			mBaseColor->GetWriteView(commandList.Get()),
			mMetallicSpecularRoughness->GetWriteView(commandList.Get()),
			mVelocity->GetWriteView(commandList.Get())
		};

		commandList->ClearRenderTargetView(rtvHandle[0], BackgroundColor, 0, nullptr);
		commandList->ClearRenderTargetView(rtvHandle[1], BackgroundColor, 0, nullptr);
		commandList->ClearRenderTargetView(rtvHandle[2], BackgroundColor, 0, nullptr);
		commandList->ClearRenderTargetView(rtvHandle[3], BackgroundColor, 0, nullptr);
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		commandList->OMSetRenderTargets(4, &rtvHandle[0], FALSE, &dsvHandle);

		commandList->SetGraphicsRootDescriptorTable(
			COMMON_SAMPLER_ROOT_TABLE_INDEX, descriptorManager->GetSamplerHeap()->GetGPUDescriptorHandleForHeapStart());

		// Structure Render
		StructureRender params = {};
		params.device = device;
		params.descriptorCache = descriptorCache;
		params.commandList = commandList.Get();

		Subject<StructureRender>* structureRender = Registry::instance().query<StructureRender>("StructureRender");
		if (structureRender)
		{
			structureRender->notify(params);
		}

		ThrowIfFailed(mCommandLists[curFrameIndex]->Close());
		ID3D12CommandList* ppCommandList[] = { mCommandLists[curFrameIndex].Get() };
		device->GetCommandQueue()->ExecuteCommandLists(1, ppCommandList);

		return true;
	}

	void GBufferPass::Destroy()
	{
		FrameGraphPass::Destroy();
	}
}