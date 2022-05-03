#pragma once
#include "Prerequisites.h"
#include "FrameGraphPass.h"
#include "FrameGraphResource.h"

namespace Amadeus
{
	class FinalPass
		: public FrameGraphPass
	{
	public:
		FinalPass(SharedPtr<DeviceResources> device);

		bool PreCompute(SharedPtr<DeviceResources> device, ID3D12GraphicsCommandList* commandList) override;

		void PostPreCompute() override;

		void Setup(FrameGraph&, FrameGraphBuilder&, FrameGraphNode*) override;

		void RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache) override;

		bool Execute(
			SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache) override;

		void Destroy() override;

	private:
		SharedPtr<FrameGraphResource> mBaseColor;

		SharedPtr<FrameGraphResource> mSky;
		SharedPtr<FrameGraphResource> mTransparent;
	};
}