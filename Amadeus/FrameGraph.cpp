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
		SharedPtr<DescriptorManager> descriptorManager,
		SharedPtr<DescriptorCache> descriptorCache, 
		SharedPtr<RenderSystem> renderer)
	{
		Vector<Future<bool>> results;

		for (auto pass : mPasses)
		{
#ifdef AMADEUS_CONCURRENCY
			results.emplace_back(
				renderer->Execute(&FrameGraphPass::Execute, pass, device, descriptorManager, descriptorCache));
#else
			pass->Execute(device, descriptorManager, descriptorCache);
#endif // AMADEUS_CONCURRENCY
		}

		for (auto&& res : results)
		{
			if (!res.get())
				throw RuntimeError("FrameGraphPass::Execute Error");
		}

		renderer->Render(device);
	}
}
