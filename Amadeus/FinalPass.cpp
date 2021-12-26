#include "pch.h"
#include <D3Dcompiler.h>
#include "FinalPass.h"
#include "RenderSystem.h"
#include "ProgramManager.h"
#include "CameraManager.h"

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
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
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
        psoDesc.SampleDesc.Count = 1;
        ThrowIfFailed(device->GetD3DDevice()->CreateGraphicsPipelineState(
            &psoDesc, 
            IID_PPV_ARGS(&mPipelineState)));

        ThrowIfFailed(device->GetD3DDevice()->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            device->GetCommandAllocator(),
            mPipelineState.Get(),
            IID_PPV_ARGS(&mCommandList)));

        ThrowIfFailed(mCommandList->Close());

        {
            Vertex triangleVertices[] =
            {
                { { -10.0f,  10.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { {  10.0f, -10.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { { -10.0f, -10.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                
                { {  10.0f, -10.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { { -10.0f,  10.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { {  10.0f,  10.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },

                { { -10.0f, -10.0f,  10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { { -10.0f,  10.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { { -10.0f, -10.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                
                { { -10.0f,  10.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { { -10.0f, -10.0f,  10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { { -10.0f,  10.0f,  10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },

                { { 10.0f, -10.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { { 10.0f,  10.0f,  10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { { 10.0f, -10.0f,  10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                
                { { 10.0f,  10.0f,  10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { { 10.0f, -10.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { { 10.0f,  10.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },

                { { -10.0f, -10.0f,  10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { {  10.0f,  10.0f,  10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { { -10.0f,  10.0f,  10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                
                { {  10.0f,  10.0f,  10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { { -10.0f, -10.0f,  10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { {  10.0f, -10.0f,  10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },

                { { -10.0f,  10.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { {  10.0f,  10.0f,  10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { {  10.0f,  10.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                
                { {  10.0f,  10.0f,  10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { { -10.0f,  10.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { { -10.0f,  10.0f,  10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },

                { { -10.0f, -10.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { {  10.0f, -10.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { { -10.0f, -10.0f,  10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                
                { {  10.0f, -10.0f, -10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { {  10.0f, -10.0f,  10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { { -10.0f, -10.0f,  10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } }
            };

            const UINT vertexBufferSize = sizeof(triangleVertices);

            const CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            const CD3DX12_RESOURCE_DESC uploadHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
            ThrowIfFailed(device->GetD3DDevice()->CreateCommittedResource(
                &uploadHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &uploadHeapDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_vertexBuffer)));

            UINT8* pVertexDataBegin;
            CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
            ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
            memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
            m_vertexBuffer->Unmap(0, nullptr);

            m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
            m_vertexBufferView.StrideInBytes = sizeof(Vertex);
            m_vertexBufferView.SizeInBytes = vertexBufferSize;
        }
	}

	void FinalPass::Setup()
	{
	}

	void FinalPass::Execute(
        SharedPtr<DeviceResources> device, 
        SharedPtr<DescriptorCache> descriptorCache, 
        SharedPtr<RenderSystem> renderer)
	{
        ThrowIfFailed(device->GetCommandAllocator()->Reset());

        ThrowIfFailed(mCommandList->Reset(device->GetCommandAllocator(), mPipelineState.Get()));

        mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
        const D3D12_VIEWPORT screenViewPort = device->GetScreenViewport();
        const D3D12_RECT scissorRect = device->GetScissorRect();
        mCommandList->RSSetViewports(1, &screenViewPort);
        mCommandList->RSSetScissorRects(1, &scissorRect);

        const CD3DX12_RESOURCE_BARRIER Present2RenderTarget = 
            CD3DX12_RESOURCE_BARRIER::Transition(
                device->GetRenderTarget(),
                D3D12_RESOURCE_STATE_PRESENT, 
                D3D12_RESOURCE_STATE_RENDER_TARGET);
        mCommandList->ResourceBarrier(1, &Present2RenderTarget);

        D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView = device->GetRenderTargetView();
        D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = device->GetDepthStencilView();
        mCommandList->OMSetRenderTargets(1, &renderTargetView, false, &depthStencilView);
        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        mCommandList->ClearRenderTargetView(renderTargetView, clearColor, 0, nullptr);

        ID3D12DescriptorHeap* ppHeaps[] = { descriptorCache->GetCbvSrvUavCache() };
        mCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        Camera& camera = CameraManager::Instance().GetDefaultCamera();
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = camera.GetCbvDesc(device);
        CD3DX12_GPU_DESCRIPTOR_HANDLE cameraConstantsHandle = descriptorCache->AppendCbvCache(device, cbvDesc);
        //mCommandList->SetGraphicsRootDescriptorTable(0, cameraConstantsHandle);
        mCommandList->SetGraphicsRootConstantBufferView(0, cbvDesc.BufferLocation);

        mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        mCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        mCommandList->DrawInstanced(36, 1, 0, 0);

        const CD3DX12_RESOURCE_BARRIER RenderTarget2Present =
            CD3DX12_RESOURCE_BARRIER::Transition(
                device->GetRenderTarget(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PRESENT);
        mCommandList->ResourceBarrier(1, &RenderTarget2Present);

        ThrowIfFailed(mCommandList->Close());

        renderer->Submit(mCommandList.Get());
	}
}