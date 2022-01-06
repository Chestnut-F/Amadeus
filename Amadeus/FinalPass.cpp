#include "pch.h"
#include <D3Dcompiler.h>
#include "FinalPass.h"
#include "RenderSystem.h"
#include "ResourceManagers.h"

namespace Amadeus
{
	FinalPass::FinalPass(SharedPtr<DeviceResources> device)
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
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(shaders.Get("VertexShader.cso"));
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(shaders.Get("PixelShader.cso"));
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
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

	void FinalPass::Setup()
	{
	}

    bool FinalPass::Execute(
        SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache)
	{
        UINT curFrameIndex = device->GetCurrentFrameIndex();
        ThrowIfFailed(device->GetCommandAllocator()->Reset());

        ThrowIfFailed(mCommandLists[curFrameIndex]->Reset(device->GetCommandAllocator(), mPipelineState.Get()));

        mCommandLists[curFrameIndex]->SetGraphicsRootSignature(mRootSignature.Get());

        ID3D12DescriptorHeap* ppHeaps[] = { descriptorCache->GetCbvSrvUavCache(device), descriptorManager->GetSamplerHeap() };
        mCommandLists[curFrameIndex]->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        const D3D12_VIEWPORT screenViewPort = device->GetScreenViewport();
        const D3D12_RECT scissorRect = device->GetScissorRect();
        mCommandLists[curFrameIndex]->RSSetViewports(1, &screenViewPort);
        mCommandLists[curFrameIndex]->RSSetScissorRects(1, &scissorRect);

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

        Camera& camera = CameraManager::Instance().GetDefaultCamera();
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = camera.GetCbvDesc(device);
        CD3DX12_GPU_DESCRIPTOR_HANDLE cameraConstantsHandle = descriptorCache->AppendCbvCache(device, cbvDesc);
        mCommandLists[curFrameIndex]->SetGraphicsRootConstantBufferView(COMMON_CAMERA_ROOT_CBV_INDEX, cbvDesc.BufferLocation);

        mCommandLists[curFrameIndex]->SetGraphicsRootDescriptorTable(COMMON_SAMPLER_ROOT_TABLE_INDEX, descriptorManager->GetSamplerHeap()->GetGPUDescriptorHandleForHeapStart());

        mCommandLists[curFrameIndex]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        MeshManager::Instance().Render(device, descriptorCache, mCommandLists[curFrameIndex].Get());

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
}