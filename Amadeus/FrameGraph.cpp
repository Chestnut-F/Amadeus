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

	FrameGraph::~FrameGraph() noexcept
	{
	}

	void FrameGraph::AddPass(const String& passName, SharedPtr<DeviceResources> device)
	{
		meta::any* pass = new meta::any(
			meta::resolve(MetaRenderPassHash(passName.c_str())).construct(device));

		FrameGraphPass* fgPass = pass->try_cast<FrameGraphPass>();
		FrameGraphNode* passNode = new FrameGraphNode(*this, fgPass);
		mPassNodes.emplace_back(passNode);
	}

	void FrameGraph::Setup()
	{
		for (auto& node : mPassNodes)
		{
			node->Setup(*this, mBuilder);
		}
	}

	void FrameGraph::Compile()
	{
		mGraph.Cull();

		auto activePassNodesEnd = std::stable_partition(
			mPassNodes.begin(), mPassNodes.end(), [](const auto& pPassNode) {
			return !pPassNode->IsCulled();
		});

		auto first = mPassNodes.begin();
		while (first != activePassNodesEnd)
		{
			const auto& passNode = *first;
			first++;
			assert(!passNode->IsCulled());

			const auto& reads = mGraph.GetIncomingEdges(passNode.get());
			for (const auto& edge : reads)
			{
				auto pNode = static_cast<FrameGraphNode*>(mGraph.GetNode(edge->from));
			}

			auto const& writes = mGraph.GetOutgoingEdges(passNode.get());
			for (auto const& edge : writes)
			{
				auto pNode = static_cast<FrameGraphNode*>(mGraph.GetNode(edge->to));
			}
		}
	}

	void FrameGraph::Execute(
		SharedPtr<DeviceResources> device, 
		SharedPtr<DescriptorManager> descriptorManager,
		SharedPtr<DescriptorCache> descriptorCache, 
		SharedPtr<RenderSystem> renderer)
	{
		Vector<Future<bool>> results;

		for (auto pass : mPassNodes)
		{
			if (!pass->IsCulled())
			{
#ifdef AMADEUS_CONCURRENCY
				results.emplace_back(
					renderer->Execute(&FrameGraphNode::Execute, pass, device, descriptorManager, descriptorCache));
#else
				pass->Execute(device, descriptorManager, descriptorCache);
#endif // AMADEUS_CONCURRENCY
			}
		}

		for (auto&& res : results)
		{
			if (!res.get())
				throw RuntimeError("FrameGraphPass::Execute Error");
		}

		renderer->Render(device);
	}

	SharedPtr<FrameGraphResource> FrameGraphBuilder::Write(
		String&& name, FrameGraphResourceType type, DXGI_FORMAT format, FrameGraph& fg, FrameGraphNode* from)
	{
		auto iter = fg.mResourcesDict.find(name);
		if (iter == fg.mResourcesDict.end())
		{
			fg.mResourcesDict[name] = std::make_shared<FrameGraphResource>(type, format);
		}
		else
		{
			auto& resource = iter->second;
			resource->SetFrom(from);

			const auto& nodes = resource->GetTo();
			for (auto& to : nodes)
			{
				resource->Connect(fg.mGraph, from, to);
			}
		}

		return fg.mResourcesDict[name];
	}

	void FrameGraphBuilder::Read(
		String&& name, FrameGraphResourceType type, DXGI_FORMAT format, FrameGraph& fg, FrameGraphNode* to)
	{
		auto iter = fg.mResourcesDict.find(name);
		if (iter == fg.mResourcesDict.end())
		{
			fg.mResourcesDict[name] = std::make_shared<FrameGraphResource>(type, format);
		}
		else
		{
			auto& resource = iter->second;
			resource->AddTo(to);

			const auto& from = resource->GetFrom();
			if (from)
			{
				DependencyGraph::Edge(fg.mGraph, from, to);
			}
		}
	}

	FrameGraphNode::FrameGraphNode(FrameGraph& fg, FrameGraphPass* pass)
		: DependencyGraph::Node(fg.mGraph)
	{
		mPass.reset(pass);
	}

	void FrameGraphNode::Setup(FrameGraph& fg, FrameGraphBuilder& builder)
	{
		mPass->Setup(fg, builder, this);
	}

	bool FrameGraphNode::Execute(
		SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache)
	{
		return mPass->Execute(device, descriptorManager, descriptorCache);
	}
}
