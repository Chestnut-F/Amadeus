#include "pch.h"
#include <meta/factory.hpp>
#include <meta/meta.hpp>
#include "RenderPassRegistry.h"
#include "FrameGraphPass.h"
#include "ShadowPass.h"
#include "GBufferPass.h"
#include "FinalPass.h"

namespace Amadeus
{
	auto ShadowPassFactory = meta::reflect<ShadowPass>(MetaRenderPassHash("ShadowPass"))
		.base<FrameGraphPass>()
		.ctor<std::shared_ptr<DeviceResources>>();

	auto GBufferPassFactory = meta::reflect<GBufferPass>(MetaRenderPassHash("GBufferPass"))
		.base<FrameGraphPass>()
		.ctor<std::shared_ptr<DeviceResources>>();

	auto FinalPassFactory = meta::reflect<FinalPass>(MetaRenderPassHash("FinalPass"))
		.base<FrameGraphPass>()
		.ctor<std::shared_ptr<DeviceResources>>();
}