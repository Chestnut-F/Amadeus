#include "pch.h"
#include "TAAPass.h"
#include "ResourceManagers.h"
#include "FrameGraph.h"

namespace Amadeus
{
	TAAPass::TAAPass(SharedPtr<DeviceResources> device)
		: FrameGraphPass::FrameGraphPass(false)
		, bFirstFrame(true)
	{
		ProgramManager& shaders = ProgramManager::Instance();

		ThrowIfFailed(device->GetD3DDevice()->CreateRootSignature(
			0,
			shaders.Get("TAARS.cso")->GetBufferPointer(),
			shaders.GetBufferSize("TAARS.cso"),
			IID_PPV_ARGS(&mRootSignature)
		));

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { nullptr, 0 };
		psoDesc.pRootSignature = mRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(shaders.Get("TAAVS.cso"));
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(shaders.Get("TAAPS.cso"));
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
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

	bool TAAPass::PreCompute(SharedPtr<DeviceResources> device, ID3D12GraphicsCommandList* commandList)
	{
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		textureDesc.Width = device->GetWindowWidth();
		textureDesc.Height = device->GetWindowHeight();
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		const CD3DX12_HEAP_PROPERTIES defaultheapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
			&defaultheapProperties,
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&mLastFrame)));
		NAME_D3D12_OBJECT(mLastFrame);

		return true;
	}

	void TAAPass::PostPreCompute()
	{
	}

	void TAAPass::Setup(FrameGraph& fg, FrameGraphBuilder& builder, FrameGraphNode* node)
	{
		mTAA = builder.Write(
			"TAABaseColor",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			fg, node);

		mPresentFrame = builder.Read(
			"BaseColor",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
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

	void TAAPass::RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache)
	{
		mTAA->RegisterResource(device, descriptorCache);
	}

	bool TAAPass::Execute(
		SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache)
	{
		FrameGraphPass::Execute(device, descriptorManager, descriptorCache);
		UINT curFrameIndex = device->GetCurrentFrameIndex();
		auto& commandList = mCommandLists[curFrameIndex];

		CD3DX12_GPU_DESCRIPTOR_HANDLE lastFrameSrvHanle = {};
		D3D12_SHADER_RESOURCE_VIEW_DESC lastFrameSrvDesc = {};
		lastFrameSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		lastFrameSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		lastFrameSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		lastFrameSrvDesc.Texture2D.MipLevels = 1;
		lastFrameSrvDesc.Texture2D.MostDetailedMip = 0;
		lastFrameSrvHanle = descriptorCache->AppendSrvCache(device, mLastFrame.Get(), lastFrameSrvDesc);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = mTAA->GetWriteView(commandList.Get());

		commandList->ClearRenderTargetView(rtvHandle, BackgroundColor, 0, nullptr);

		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		commandList->SetGraphicsRootDescriptorTable(
			TAA_SHADER_RESOURCE_CURR_FRAME_INDEX, mPresentFrame->GetReadView(commandList.Get()));
		commandList->SetGraphicsRootDescriptorTable(
			TAA_SHADER_RESOURCE_PREV_FRAME_INDEX, lastFrameSrvHanle);
		commandList->SetGraphicsRootDescriptorTable(
			TAA_SHADER_RESOURCE_VELOCITY_INDEX, mVelocity->GetReadView(commandList.Get()));
		commandList->SetGraphicsRootDescriptorTable(
			TAA_SHADER_RESOURCE_DEPTH_INDEX, mDepth->GetReadView(commandList.Get()));

		// TAA Render
		TAARender params = {};
		params.device = device;
		params.descriptorCache = descriptorCache;
		params.commandList = commandList.Get();

		Subject<TAARender>* taaRender = Registry::instance().query<TAARender>("TAARender");
		if (taaRender)
		{
			taaRender->notify(params);
		}

		commandList->IASetVertexBuffers(0, 0, nullptr);
		commandList->IASetIndexBuffer(nullptr);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->DrawInstanced(6, 1, 0, 0);

		const CD3DX12_RESOURCE_BARRIER RenderTarget2CopySource =
			CD3DX12_RESOURCE_BARRIER::Transition(
				mTAA->GetResource(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_COPY_SOURCE);
		commandList->ResourceBarrier(1, &RenderTarget2CopySource);
		const CD3DX12_RESOURCE_BARRIER ShaderResource2CopyDest =
			CD3DX12_RESOURCE_BARRIER::Transition(
				mLastFrame.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_COPY_DEST);
		commandList->ResourceBarrier(1, &ShaderResource2CopyDest);

		commandList->CopyResource(mLastFrame.Get(), mTAA->GetResource());

		const CD3DX12_RESOURCE_BARRIER CopySource2RenderTarget =
			CD3DX12_RESOURCE_BARRIER::Transition(
				mTAA->GetResource(),
				D3D12_RESOURCE_STATE_COPY_SOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ResourceBarrier(1, &CopySource2RenderTarget);
		const CD3DX12_RESOURCE_BARRIER CopyDest2ShaderResource =
			CD3DX12_RESOURCE_BARRIER::Transition(
				mLastFrame.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->ResourceBarrier(1, &CopyDest2ShaderResource);

		ThrowIfFailed(mCommandLists[curFrameIndex]->Close());
		ID3D12CommandList* ppCommandList[] = { mCommandLists[curFrameIndex].Get()};
		device->GetCommandQueue()->ExecuteCommandLists(1, ppCommandList);


		bFirstFrame = false;
		return true;
	}

	void TAAPass::Destroy()
	{
		mLastFrame->Release();

		FrameGraphPass::Destroy();
	}
}