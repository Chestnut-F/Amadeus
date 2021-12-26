#include "pch.h"
#include "FrameGraph.h"
#include <meta/factory.hpp>
#include <meta/meta.hpp>
#include "RenderPassRegistry.h"
#include "RenderSystem.h"

namespace Amadeus
{
	FrameGraph::FrameGraph()
	{
	}

	void FrameGraph::AddPass(const String& passName, SharedPtr<DeviceResources> device)
	{
		meta::any* pass = new meta::any(
			meta::resolve(MetaRenderPassHash(passName.c_str())).construct(device));
		mPasses.emplace_back(pass->try_cast<FrameGraphPass>());
	}

	void FrameGraph::Execute(
		SharedPtr<DeviceResources> device, 
		SharedPtr<DescriptorCache> descriptorCache, 
		SharedPtr<RenderSystem> renderer)
	{
		for (auto pass : mPasses)
		{
			pass->Execute(device, descriptorCache, renderer);
		}

		renderer->Render(device);
	}
}
