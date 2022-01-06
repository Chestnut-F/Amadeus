#pragma once
#include "Prerequisites.h"

namespace Amadeus
{
	namespace Gltf
	{
		void LoadGltf(WString&& fileName, SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager);
	}
}