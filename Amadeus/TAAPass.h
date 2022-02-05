#pragma once
#include "Prerequisites.h"
#include "FrameGraphPass.h"
#include "FrameGraphResource.h"

namespace Amadeus
{
	static constexpr UINT TAA_SHADER_RESOURCE_CURR_FRAME_INDEX = 1;
	static constexpr UINT TAA_SHADER_RESOURCE_PREV_FRAME_INDEX = 2;
	static constexpr UINT TAA_SHADER_RESOURCE_VELOCITY_INDEX = 3;
	static constexpr UINT TAA_SHADER_RESOURCE_DEPTH_INDEX = 4;

	class TAAPass
		: public FrameGraphPass
	{
	public:
		TAAPass(SharedPtr<DeviceResources> device);

		bool PreCompute(SharedPtr<DeviceResources> device, ID3D12GraphicsCommandList* commandList) override;

		void PostPreCompute() override;

		void Setup(FrameGraph& fg, FrameGraphBuilder& builder, FrameGraphNode* node) override;

		void RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache) override;

		bool Execute(SharedPtr<DeviceResources> device,
			SharedPtr<DescriptorManager> descriptorManager,
			SharedPtr<DescriptorCache> descriptorCache) override;

		void Destroy() override;

	private:
		SharedPtr<FrameGraphResource> mPresentFrame;
		SharedPtr<FrameGraphResource> mVelocity;
		SharedPtr<FrameGraphResource> mDepth;
		SharedPtr<FrameGraphResource> mTAA;

		ComPtr<ID3D12Resource> mLastFrame;
		bool bFirstFrame;
	};
}