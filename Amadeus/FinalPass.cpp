#include "pch.h"
#include <D3Dcompiler.h>
#include "FinalPass.h"
#include "RenderSystem.h"
#include "ResourceManagers.h"
#include "FrameGraph.h"

namespace Amadeus
{
	FinalPass::FinalPass(SharedPtr<DeviceResources> device)
        : FrameGraphPass::FrameGraphPass(true)
	{
        ProgramManager& shaders = ProgramManager::Instance();

        ThrowIfFailed(device->GetD3DDevice()->CreateRootSignature(
            0,
            shaders.Get("Common.cso")->GetBufferPointer(),
            shaders.GetBufferSize("Common.cso"),
            IID_PPV_ARGS(&mRootSignature)
        ));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { nullptr, 0 };
        psoDesc.pRootSignature = mRootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(shaders.Get("VertexShader.cso"));
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(shaders.Get("PixelShader.cso"));
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
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

    bool FinalPass::PreCompute(SharedPtr<DeviceResources> device, ID3D12GraphicsCommandList* commandList)
    {
        return true;
    }

    void FinalPass::PostPreCompute()
    {
    }

    void FinalPass::Setup(FrameGraph& fg, FrameGraphBuilder& builder, FrameGraphNode* node)
    {
        mBaseColor = builder.Read(
            "BaseColor",
            FrameGraphResourceType::RENDER_TARGET,
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            fg, node);
    }

    void FinalPass::RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache)
    {
    }

    bool FinalPass::Execute(SharedPtr<DeviceResources> device, 
        SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache)
	{
        FrameGraphPass::Execute(device, descriptorManager, descriptorCache);
        UINT curFrameIndex = device->GetCurrentFrameIndex();

        const CD3DX12_RESOURCE_BARRIER Present2RenderTarget = 
            CD3DX12_RESOURCE_BARRIER::Transition(
                device->GetRenderTarget(),
                D3D12_RESOURCE_STATE_PRESENT, 
                D3D12_RESOURCE_STATE_RENDER_TARGET);
        mCommandLists[curFrameIndex]->ResourceBarrier(1, &Present2RenderTarget);

        D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView = device->GetRenderTargetView();
        D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = device->GetDepthStencilView();
        mCommandLists[curFrameIndex]->OMSetRenderTargets(1, &renderTargetView, false, &depthStencilView);
        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        mCommandLists[curFrameIndex]->ClearRenderTargetView(renderTargetView, clearColor, 0, nullptr);
        mCommandLists[curFrameIndex]->ClearDepthStencilView(depthStencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle = mBaseColor->GetReadView(mCommandLists[curFrameIndex].Get());
        mCommandLists[curFrameIndex]->SetGraphicsRootDescriptorTable(5, srvHandle);

        mCommandLists[curFrameIndex]->IASetVertexBuffers(0, 0, nullptr);
        mCommandLists[curFrameIndex]->IASetIndexBuffer(nullptr);
        mCommandLists[curFrameIndex]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        mCommandLists[curFrameIndex]->DrawInstanced(6, 1, 0, 0);

        const CD3DX12_RESOURCE_BARRIER RenderTarget2Present =
            CD3DX12_RESOURCE_BARRIER::Transition(
                device->GetRenderTarget(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PRESENT);
        mCommandLists[curFrameIndex]->ResourceBarrier(1, &RenderTarget2Present);

        ThrowIfFailed(mCommandLists[curFrameIndex]->Close());

        ID3D12CommandList* ppCommandList[] = { mCommandLists[curFrameIndex].Get() };
        device->GetCommandQueue()->ExecuteCommandLists(1, ppCommandList);

        return true;
	}

    void FinalPass::Destroy()
    {
        FrameGraphPass::Destroy();
    }
}