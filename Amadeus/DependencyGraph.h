#pragma once
#include "Prerequisites.h"

namespace Amadeus
{
	class DependencyGraph
	{
	public:
		DependencyGraph() noexcept;
		~DependencyGraph() noexcept = default;
		DependencyGraph(const DependencyGraph&) noexcept = delete;
		DependencyGraph& operator=(const DependencyGraph&) noexcept = delete;

		class Node;
		typedef uint32_t NodeId;

		struct Edge
		{
			const NodeId from;
			const NodeId to;

			Edge(DependencyGraph& graph, Node* from, Node* to) noexcept;

			Edge(const Edge&) noexcept = delete;
			Edge& operator=(const Edge&) noexcept = delete;
		};

		class Node
		{
		public:
			Node(DependencyGraph& graph) noexcept;
			Node(Node&&) noexcept = default;
			virtual ~Node() noexcept = default;

			Node(const Node&) noexcept = delete;
			Node& operator=(const Node&) noexcept = delete;

			NodeId GetId() const noexcept { return mId; }
			void MakeTarget() noexcept { mRefCount = TARGET; }
			bool IsTarget() const noexcept { return mRefCount >= TARGET; }
			bool IsCulled() const noexcept { return mRefCount == 0; }
			uint32_t GetRefCount() const noexcept { return (mRefCount & TARGET) ? 1u : mRefCount; }

		private:
			friend class DependencyGraph;
			static const constexpr uint32_t TARGET = 0x80000000u;
			uint32_t mRefCount = 0;
			const NodeId mId;
		};

		typedef std::vector<Edge*> EdgeContainer;
		typedef std::vector<Node*> NodeContainer;

		Node* GetNode(NodeId id) noexcept;
		const Node* GetNode(NodeId id) const noexcept;
		EdgeContainer GetIncomingEdges(const Node* node) noexcept;
		EdgeContainer GetOutgoingEdges(const Node* node) noexcept;

		void Cull() noexcept;

		bool IsEdgeValid(const Edge* edge) const noexcept;
		bool IsAcyclic() const noexcept;

	private:
		uint32_t GenerateNodeId() noexcept;
		void RegisterNode(Node* node, NodeId id) noexcept;
		void Link(Edge* edge) noexcept;
		bool _IsAcyclic() const noexcept;
		NodeContainer mNodes;
		EdgeContainer mEdges;
	};
}