#include "pch.h"
#include <meta/factory.hpp>
#include <meta/meta.hpp>
#include "RenderPassRegistry.h"
#include "FrameGraphPass.h"
#include "GBufferPass.h"
#include "FinalPass.h"

namespace Amadeus
{
	auto GBufferPassFactory = meta::reflect<GBufferPass>(MetaRenderPassHash("GBufferPass"))
		.base<FrameGraphPass>()
		.ctor<std::shared_ptr<DeviceResources>>();

	auto FinalPassFactory = meta::reflect<FinalPass>(MetaRenderPassHash("FinalPass"))
		.base<FrameGraphPass>()
		.ctor<std::shared_ptr<DeviceResources>>();
}