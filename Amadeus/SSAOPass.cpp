#include "pch.h"
#include <random>
#include "SSAOPass.h"
#include "ResourceManagers.h"
#include "FrameGraph.h"

namespace Amadeus
{
    SSAOPass::SSAOPass(SharedPtr<DeviceResources> device)
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
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(shaders.Get("SSAO.cso"));
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

    bool SSAOPass::PreCompute(SharedPtr<DeviceResources> device, ID3D12GraphicsCommandList* commandList)
    {
        mSSAOKernel = new SSAOKernelConstantBuffer();
        mSSAONoise = new SSAONoiseConstantBuffer();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(0.0, 1.0);
        for (uint32_t i = 0; i < SSAOKernelSize; ++i)
        {
            XMFLOAT3 sample(dis(gen) * 2.0f - 1.0f, dis(gen) * 2.0f - 1.0f, dis(gen));
            XMVECTOR sampleVector = XMVector3Normalize(XMLoadFloat3(&sample));
            sampleVector *= dis(gen);
            float scale = float(i) / float(SSAOKernelSize);
            scale = 0.1f + scale * scale * (1.0f - 0.1f);
            XMStoreFloat3(&mSSAOKernel->ssaoKernel[i], sampleVector * scale);
        }

        const UINT kernelConstantBufferSize = sizeof(SSAOKernelConstantBuffer);
        const CD3DX12_HEAP_PROPERTIES defaultheapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        const CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const CD3DX12_RESOURCE_DESC kernelConstantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kernelConstantBufferSize);

        ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &kernelConstantBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mSSAOConstantBuffer)));
        NAME_D3D12_OBJECT(mSSAOConstantBuffer);

        // Map and initialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay.
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(mSSAOConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pSSAOCbvDataBegin)));
        memcpy(pSSAOCbvDataBegin, mSSAOKernel, kernelConstantBufferSize);
        mSSAOConstantBuffer->Unmap(0, nullptr);

        // Random noise
        {
            for (uint32_t i = 0; i < SSAONoiseWidth * SSAONoiseHeight; i++)
            {
                mSSAONoise->ssaoNoise[i] = XMFLOAT3(
                    dis(gen) * 2.0f - 1.0f,
                    dis(gen) * 2.0f - 1.0f,
                    0.0f);
            }

            D3D12_RESOURCE_DESC textureDesc = {};
            textureDesc.MipLevels = 1;
            textureDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
            textureDesc.Width = SSAONoiseWidth;
            textureDesc.Height = SSAONoiseHeight;
            textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
            textureDesc.DepthOrArraySize = 1;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

            ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
                &defaultheapProperties,
                D3D12_HEAP_FLAG_NONE,
                &textureDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&mSSAONoiseTexture)));

            const UINT64 uploadBufferSize = GetRequiredIntermediateSize(mSSAONoiseTexture.Get(), 0, 1);
            const CD3DX12_RESOURCE_DESC uploadHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
            ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
                &uploadHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &uploadHeapDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&mSSAONoiseUploadHeap)));

            D3D12_SUBRESOURCE_DATA textureData = {};
            textureData.pData = &mSSAONoise->ssaoNoise[0];
            textureData.RowPitch = SSAONoiseWidth * sizeof(XMFLOAT3);
            textureData.SlicePitch = textureData.RowPitch * SSAONoiseHeight;

            UpdateSubresources(commandList, mSSAONoiseTexture.Get(), mSSAONoiseUploadHeap.Get(), 0, 0, 1, &textureData);
            const CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
                mSSAONoiseTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            commandList->ResourceBarrier(1, &resourceBarrier);
        }

        commandList->Close();

        ID3D12CommandList* ppCommandLists[] = { commandList };
        device->GetCommandQueue()->ExecuteCommandLists(1, ppCommandLists);

        return true;
    }

    void SSAOPass::PostPreCompute()
    {
        mSSAONoiseUploadHeap->Release();
        mSSAONoiseUploadHeap.Reset();
    }

    void SSAOPass::Setup(FrameGraph& fg, FrameGraphBuilder& builder, FrameGraphNode* node)
    {
		mSSAO = builder.Write(
			"SSAO",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R32_FLOAT,
			fg, node);

		mZPrePosition = builder.Read(
			"ZPrePosition",
			FrameGraphResourceType::RENDER_TARGET,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
			fg, node);

		mZPreNormal = builder.Read(
			"ZPreNormal",
			FrameGraphResourceType::RENDER_TARGET,
			DXGI_FORMAT_R11G11B10_FLOAT,
			fg, node);
    }

    void SSAOPass::RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache)
    {
		mSSAO->RegisterResource(device, descriptorCache);
    }

    bool SSAOPass::Execute(
        SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache)
    {
		FrameGraphPass::Execute(device, descriptorManager, descriptorCache);
		UINT curFrameIndex = device->GetCurrentFrameIndex();
		auto& commandList = mCommandLists[curFrameIndex];

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = mSSAO->GetWriteView(commandList.Get());
		commandList->ClearRenderTargetView(rtvHandle, BackgroundColor, 0, nullptr);

		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        commandList->SetGraphicsRootConstantBufferView(
            SSAO_CONSTANT_BUFFER_KERNEL_INDEX, mSSAOConstantBuffer->GetGPUVirtualAddress());
		commandList->SetGraphicsRootDescriptorTable(
            SSAO_SHADER_RESOURCE_POSITION_INDEX, mZPrePosition->GetReadView(commandList.Get()));
		commandList->SetGraphicsRootDescriptorTable(
            SSAO_SHADER_RESOURCE_NORMAL_INDEX, mZPreNormal->GetReadView(commandList.Get()));

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        D3D12_GPU_DESCRIPTOR_HANDLE noiseHandle = descriptorCache->AppendSrvCache(device, mSSAONoiseTexture.Get(), srvDesc);
		commandList->SetGraphicsRootDescriptorTable(SSAO_SHADER_RESOURCE_NOISE_INDEX, noiseHandle);

		// SSAO Render
		SSAORender params = {};
		params.device = device;
		params.descriptorCache = descriptorCache;
		params.commandList = commandList.Get();

		Subject<SSAORender>* ssaoRender = Registry::instance().query<SSAORender>("SSAORender");
		if (ssaoRender)
		{
			ssaoRender->notify(params);
		}

        commandList->IASetVertexBuffers(0, 0, nullptr);
        commandList->IASetIndexBuffer(nullptr);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawInstanced(6, 1, 0, 0);

		ThrowIfFailed(mCommandLists[curFrameIndex]->Close());
		ID3D12CommandList* ppCommandList[] = { mCommandLists[curFrameIndex].Get() };
		device->GetCommandQueue()->ExecuteCommandLists(1, ppCommandList);

        return true;
    }

    void SSAOPass::Destroy()
    {
        mSSAOConstantBuffer->Release();
        mSSAONoiseTexture->Release();

        FrameGraphPass::Destroy();
    }
}