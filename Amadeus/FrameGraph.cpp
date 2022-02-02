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
		if (fgPass->IsTarget())
		{
			passNode->MakeTarget();
		}

		mPassNodes.emplace_back(passNode);
	}

	void FrameGraph::PreCompute(SharedPtr<DeviceResources> device, SharedPtr<RenderSystem> renderer)
	{
		Vector<ID3D12Resource*> uploadHeaps;
		Vector<ID3D12GraphicsCommandList*> commandLists;
		Vector<ID3D12CommandAllocator*> commandAllocators;

		Vector<Future<bool>> results;
		for (auto& pass : mPassNodes)
		{
			ID3D12CommandAllocator* commandAllocator = {};

			ThrowIfFailed(device->GetD3DDevice()->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(&commandAllocator)));

			ID3D12GraphicsCommandList* commandList = {};

			ThrowIfFailed(device->GetD3DDevice()->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				commandAllocator,
				nullptr,
				IID_PPV_ARGS(&commandList)));

			commandAllocators.emplace_back(commandAllocator);
			commandLists.emplace_back(commandList);

#ifdef AMADEUS_CONCURRENCY
			results.emplace_back(
				renderer->Execute(&FrameGraphNode::PreCompute, pass.get(), device, commandList));
#else
			pass->PreCompute(device, uploadHeap, commandList);
#endif // AMADEUS_CONCURRENCY
		}

		for (auto&& res : results)
		{
			if (!res.get())
				throw RuntimeError("FrameGraphPass::PreCompute Error");
		}

		renderer->Upload(device);

		for (auto& pass : mPassNodes)
		{
			pass->PostPreCompute();
		}

		for (auto&& commandAllocator : commandAllocators)
		{
			commandAllocator->Release();
		}
		commandAllocators.clear();

		for (auto&& commandList : commandLists)
		{
			commandList->Release();
		}
		commandLists.clear();
	}

	void FrameGraph::Setup()
	{
		for (auto& node : mPassNodes)
		{
			node->Setup(*this, mBuilder);
		}
	}

	void FrameGraph::Compile(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache)
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
				pNode->RegisterResource(device, descriptorCache);
			}

			auto const& writes = mGraph.GetOutgoingEdges(passNode.get());
			for (auto const& edge : writes)
			{
				auto pNode = static_cast<FrameGraphNode*>(mGraph.GetNode(edge->to));
				pNode->RegisterResource(device, descriptorCache);
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
		ThrowIfFailed(device->GetCommandAllocator()->Reset());

		auto activePassNodesEnd = std::find_if(mPassNodes.begin(), mPassNodes.end(), [](const auto& pPassNode) {
			return pPassNode->IsCulled();
		});

		auto iter = mPassNodes.begin();
		for (; iter != activePassNodesEnd; ++iter)
		{
			assert(!(*iter)->IsCulled());

#ifdef AMADEUS_CONCURRENCY
			results.emplace_back(
				renderer->Execute(&FrameGraphNode::Execute, iter->get(), device, descriptorManager, descriptorCache));
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

	void FrameGraph::Destroy()
	{
		for (auto& pass : mPassNodes)
		{
			pass->Destroy();
		}
		mPassNodes.clear();

		for (auto& resource : mResourcesDict)
		{
			resource.second->Destroy();
		}
		mResourcesDict.clear();
	}

	FrameGraphNode* FrameGraphBuilder::DeclarePass(FrameGraph& fg, FrameGraphPass* pass)
	{
		return nullptr;
	}

	SharedPtr<FrameGraphResource> FrameGraphBuilder::Write(
		String&& name, FrameGraphResourceType type, DXGI_FORMAT format, FrameGraph& fg, FrameGraphNode* from)
	{
		auto iter = fg.mResourcesDict.find(name);
		if (iter == fg.mResourcesDict.end())
		{
			fg.mResourcesDict[name] = std::make_shared<FrameGraphResource>(name, type, format, from);
		}

		return fg.mResourcesDict[name];
	}

	SharedPtr<FrameGraphResource> FrameGraphBuilder::Read(
		String&& name, FrameGraphResourceType type, DXGI_FORMAT format, FrameGraph& fg, FrameGraphNode* to)
	{
		auto iter = fg.mResourcesDict.find(name);
		if (iter == fg.mResourcesDict.end())
		{
			throw Exception("Need Write Frame Graph Resource Before Read.");
		}
		else
		{
			auto& resource = iter->second;
			resource->Connect(fg.mGraph, to);
		}

		return fg.mResourcesDict[name];
	}

	FrameGraphNode::FrameGraphNode(FrameGraph& fg, FrameGraphPass* pass)
		: DependencyGraph::Node(fg.mGraph)
	{
		mPass.reset(pass);
	}

	bool FrameGraphNode::PreCompute(
		SharedPtr<DeviceResources> device, ID3D12GraphicsCommandList* commandList)
	{
		return mPass->PreCompute(device, commandList);
	}

	void FrameGraphNode::PostPreCompute()
	{
		mPass->PostPreCompute();
	}

	void FrameGraphNode::Setup(FrameGraph& fg, FrameGraphBuilder& builder)
	{
		mPass->Setup(fg, builder, this);
	}

	void FrameGraphNode::RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache)
	{
		mPass->RegisterResource(device, descriptorCache);
	}

	bool FrameGraphNode::Execute(
		SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache)
	{
		return mPass->Execute(device, descriptorManager, descriptorCache);
	}

	void FrameGraphNode::Destroy()
	{
		mPass->Destroy();
	}
}
