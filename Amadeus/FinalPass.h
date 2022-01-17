#pragma once
#include "Prerequisites.h"
#include "FrameGraphPass.h"

namespace Amadeus
{
	class FinalPass
		: public FrameGraphPass
	{
	public:
		FinalPass(SharedPtr<DeviceResources> device);

		void Setup(FrameGraph&, FrameGraphBuilder&, FrameGraphNode*) override;

		void RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache) override;

		bool Execute(
			SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache) override;

	private:
		SharedPtr<FrameGraphResource> mBaseColor;
	};
}