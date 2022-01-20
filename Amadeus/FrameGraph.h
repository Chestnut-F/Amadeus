#pragma once
#include "Prerequisites.h"
#include "DependencyGraph.h"
#include "FrameGraphPass.h"
#include "FrameGraphResource.h"

namespace Amadeus
{
	class FrameGraph;
	class FrameGraphBuilder;
	class FrameGraphNode;

	class FrameGraphBuilder
	{
	public:
		FrameGraphNode* DeclarePass(FrameGraph& fg, FrameGraphPass* pass);

		SharedPtr<FrameGraphResource> Write(
			String&& name, FrameGraphResourceType type, DXGI_FORMAT format, FrameGraph& fg, FrameGraphNode* from);

		SharedPtr<FrameGraphResource> Read(
			String&& name, FrameGraphResourceType type, DXGI_FORMAT format, FrameGraph& fg, FrameGraphNode* to);
	};

	class FrameGraphNode
		: public DependencyGraph::Node
	{
	public:
		FrameGraphNode(FrameGraph& fg, FrameGraphPass* pass);

		void Setup(FrameGraph& fg, FrameGraphBuilder& builder);

		void RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache);

		bool Execute(
			SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache);

		void Destroy();

	private:
		UniquePtr<FrameGraphPass> mPass;
	};

	class FrameGraph
	{
	public:
		explicit FrameGraph();
		FrameGraph(const FrameGraph&) = delete;
		FrameGraph& operator=(const FrameGraph&) = delete;
		~FrameGraph() noexcept;

		void AddPass(const String& passName, SharedPtr<DeviceResources> device);

		void Setup();

		void Compile(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache);

		void Execute(SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache, SharedPtr<RenderSystem> renderer);

		void Destroy();

	private:
		friend class FrameGraphBuilder;
		friend class FrameGraphNode;

		DependencyGraph mGraph;

		FrameGraphBuilder mBuilder;
		Vector<SharedPtr<FrameGraphNode>> mPassNodes;
		Map<String, SharedPtr<FrameGraphResource>> mResourcesDict;
	};
}