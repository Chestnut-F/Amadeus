#include "pch.h"
#include <meta/factory.hpp>
#include <meta/meta.hpp>
#include "RenderPassRegistry.h"
#include "FrameGraphPass.h"
#include "ShadowPass.h"
#include "ZPrePass.h"
#include "SSAOPass.h"
#include "SSAOBlurPass.h"
#include "GBufferPass.h"
#include "TAAPass.h"
#include "FinalPass.h"

namespace Amadeus
{
	auto ShadowPassFactory = meta::reflect<ShadowPass>(MetaRenderPassHash("ShadowPass"))
		.base<FrameGraphPass>()
		.ctor<std::shared_ptr<DeviceResources>>();

	auto ZPrePassFactory = meta::reflect<ZPrePass>(MetaRenderPassHash("ZPrePass"))
		.base<FrameGraphPass>()
		.ctor<std::shared_ptr<DeviceResources>>();

	auto SSAOPassFactory = meta::reflect<SSAOPass>(MetaRenderPassHash("SSAOPass"))
		.base<FrameGraphPass>()
		.ctor<std::shared_ptr<DeviceResources>>();

	auto SSAOBlurPassFactory = meta::reflect<SSAOBlurPass>(MetaRenderPassHash("SSAOBlurPass"))
		.base<FrameGraphPass>()
		.ctor<std::shared_ptr<DeviceResources>>();

	auto GBufferPassFactory = meta::reflect<GBufferPass>(MetaRenderPassHash("GBufferPass"))
		.base<FrameGraphPass>()
		.ctor<std::shared_ptr<DeviceResources>>();

	auto TAAPassFactory = meta::reflect<TAAPass>(MetaRenderPassHash("TAAPass"))
		.base<FrameGraphPass>()
		.ctor<std::shared_ptr<DeviceResources>>();

	auto FinalPassFactory = meta::reflect<FinalPass>(MetaRenderPassHash("FinalPass"))
		.base<FrameGraphPass>()
		.ctor<std::shared_ptr<DeviceResources>>();
}