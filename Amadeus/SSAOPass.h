#pragma once
#include "Prerequisites.h"
#include "FrameGraphPass.h"
#include "FrameGraphResource.h"

namespace Amadeus
{
	static constexpr UINT SSAO_CONSTANT_BUFFER_KERNEL_INDEX = 1;
	static constexpr UINT SSAO_SHADER_RESOURCE_POSITION_INDEX = 2;
	static constexpr UINT SSAO_SHADER_RESOURCE_NORMAL_INDEX = 3;
	static constexpr UINT SSAO_SHADER_RESOURCE_NOISE_INDEX = 4;

	static const UINT SSAOKernelSize = 64;
	static const UINT SSAONoiseWidth = 4;
	static const UINT SSAONoiseHeight = 4;

	struct SSAOKernelConstantBuffer
	{
		XMFLOAT3 ssaoKernel[SSAOKernelSize];
	};
	static_assert((sizeof(SSAOKernelConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

	struct SSAONoiseConstantBuffer
	{
		XMFLOAT3 ssaoNoise[SSAONoiseWidth * SSAONoiseHeight];
	};
	static_assert((sizeof(SSAOKernelConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

	class SSAOPass
		: public FrameGraphPass
	{
	public:
		SSAOPass(SharedPtr<DeviceResources> device);

		bool PreCompute(SharedPtr<DeviceResources> device, ID3D12GraphicsCommandList* commandList) override;

		void PostPreCompute() override;

		void Setup(FrameGraph& fg, FrameGraphBuilder& builder, FrameGraphNode* node) override;

		void RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache) override;

		bool Execute(SharedPtr<DeviceResources> device,
			SharedPtr<DescriptorManager> descriptorManager,
			SharedPtr<DescriptorCache> descriptorCache) override;

		void Destroy() override;

	private:
		SharedPtr<FrameGraphResource> mZPrePosition;
		SharedPtr<FrameGraphResource> mZPreNormal;

		SharedPtr<FrameGraphResource> mSSAO;

		UINT8* pSSAOCbvDataBegin;
		ComPtr<ID3D12Resource> mSSAOConstantBuffer;
		SSAOKernelConstantBuffer* mSSAOKernel;

		ComPtr<ID3D12Resource> mSSAONoiseUploadHeap;
		ComPtr<ID3D12Resource> mSSAONoiseTexture;
		SSAONoiseConstantBuffer* mSSAONoise;
	};
}