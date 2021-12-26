#include "pch.h"
#include <meta/factory.hpp>
#include <meta/meta.hpp>
#include "RenderPassRegistry.h"

namespace Amadeus
{
	auto FinalPassFactory = meta::reflect<FinalPass>(MetaRenderPassHash("FinalPass"))
		.base<FrameGraphPass>()
		.ctor<std::shared_ptr<DeviceResources>>()
		.func<&FinalPass::Setup>(MetaRenderPassHash("Setup"))
		.func<&FinalPass::Execute>(MetaRenderPassHash("Execute"));
}