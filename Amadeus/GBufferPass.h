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

		void Setup(FrameGraph& fg, FrameGraphBuilder& builder, FrameGraphNode* node) override;

		bool Execute(SharedPtr<DeviceResources> device, 
			SharedPtr<DescriptorManager> descriptorManager, 
			SharedPtr<DescriptorCache> descriptorCache) override;

	private:
		SharedPtr<FrameGraphResource> mNormal;
		SharedPtr<FrameGraphResource> mBaseColor;
		SharedPtr<FrameGraphResource> mMetallicSpecularRoughness;
		SharedPtr<FrameGraphResource> mVelocity;
		SharedPtr<FrameGraphResource> mDepth;
	};
}