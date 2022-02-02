#pragma once
#include "Prerequisites.h"
#include "FrameGraphPass.h"
#include "FrameGraphResource.h"

namespace Amadeus
{
	class GBufferPass
		: public FrameGraphPass
	{
	public:
		GBufferPass(SharedPtr<DeviceResources> device);

		bool PreCompute(SharedPtr<DeviceResources> device, ID3D12GraphicsCommandList* commandList) override;

		void PostPreCompute() override;

		void Setup(FrameGraph& fg, FrameGraphBuilder& builder, FrameGraphNode* node) override;

		void RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache) override;

		bool Execute(SharedPtr<DeviceResources> device, 
			SharedPtr<DescriptorManager> descriptorManager, 
			SharedPtr<DescriptorCache> descriptorCache) override;

		void Destroy() override;

	private:
		SharedPtr<FrameGraphResource> mNormal;
		SharedPtr<FrameGraphResource> mBaseColor;
		SharedPtr<FrameGraphResource> mMetallicSpecularRoughness;
		SharedPtr<FrameGraphResource> mVelocity;
		SharedPtr<FrameGraphResource> mDepth;

		SharedPtr<FrameGraphResource> mShadowMap;
		SharedPtr<FrameGraphResource> mSSAO;
	};
}