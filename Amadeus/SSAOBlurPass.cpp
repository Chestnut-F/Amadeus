#include "pch.h"
#include "SSAOBlurPass.h"
#include "ResourceManagers.h"
#include "FrameGraph.h"

namespace Amadeus
{
	SSAOBlurPass::SSAOBlurPass(SharedPtr<DeviceResources> device)
		: FrameGraphPass::FrameGraphPass(false)
	{
		ProgramManager& shaders = ProgramManager::Instance();

		ThrowIfFailed(device->GetD3DDevice()->CreateRootSignature(
			0,
			shaders.Get("ScreenSpaceRS.cso")->GetBufferPointer(),
			shaders.GetBufferSize("ScreenSpaceRS.cso"),
			IID_PPV_ARGS(&mRootSignature)
		));

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { nullptr, 0 };
		psoDesc.pRootSignature = mRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(shaders.Get("ScreenSpaceVS.cso"));
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(shaders.Get("SSAOBlur.cso"));
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R32_FLOAT;
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

	bool SSAOBlurPass::PreCompute(SharedPtr<DeviceResources> device, ID3D12GraphicsCommandList* commandList)
	{
		return true;
	}

	void SSAOBlurPass::PostPreCompute()
	{
	}

	void SSAOBlurPass::Setup(FrameGraph& fg, FrameGraphBuilder& builder, FrameGraphNode* node)
	{
		mSSAO = builder.Read(
			"SSAO",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R32_FLOAT,
			fg, node);

		mSSAOBlur = builder.Write(
			"SSAOBlur",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R32_FLOAT,
			fg, node);
	}

	void SSAOBlurPass::RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache)
	{
		mSSAOBlur->RegisterResource(device, descriptorCache);
	}

	bool SSAOBlurPass::Execute(SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache)
	{
		FrameGraphPass::Execute(device, descriptorManager, descriptorCache);
		UINT curFrameIndex = device->GetCurrentFrameIndex();
		auto& commandList = mCommandLists[curFrameIndex];

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = mSSAOBlur->GetWriteView(commandList.Get());
		commandList->ClearRenderTargetView(rtvHandle, BackgroundColor, 0, nullptr);

		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		commandList->SetGraphicsRootDescriptorTable(
			SSAO_SHADER_RESOURCE_SSAO_INDEX, mSSAO->GetReadView(commandList.Get()));

		commandList->IASetVertexBuffers(0, 0, nullptr);
		commandList->IASetIndexBuffer(nullptr);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->DrawInstanced(6, 1, 0, 0);

		ThrowIfFailed(mCommandLists[curFrameIndex]->Close());
		ID3D12CommandList* ppCommandList[] = { mCommandLists[curFrameIndex].Get() };
		device->GetCommandQueue()->ExecuteCommandLists(1, ppCommandList);

		return true;
	}

	void SSAOBlurPass::Destroy()
	{
		FrameGraphPass::Destroy();
	}
}