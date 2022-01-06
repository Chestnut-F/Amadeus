#pragma once
#include "Prerequisites.h"

namespace Amadeus
{
	class FrameGraph
	{
	public:
		FrameGraph();

		void AddPass(const String& passName, SharedPtr<DeviceResources> device);

		void Execute(SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache, SharedPtr<RenderSystem> renderer);

	private:
		Vector<FrameGraphPass*> mPasses;
	};
}