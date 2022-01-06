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

		void Setup() override;

		bool Execute(
			SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache) override;

	private:
	};
}