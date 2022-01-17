#include "pch.h"
#include "DependencyGraph.h"

namespace Amadeus
{
	DependencyGraph::DependencyGraph() noexcept
	{
		mNodes.reserve(8);
		mEdges.reserve(16);
	}

	DependencyGraph::Edge::Edge(DependencyGraph& graph, Node* from, Node* to) noexcept
		: from(from->GetId())
		, to(to->GetId())
	{
		assert(graph.mNodes[this->from] == from);
		assert(graph.mNodes[this->to] == to);
		graph.Link(this);
	}

	DependencyGraph::Node::Node(DependencyGraph& graph) noexcept
		: mId(graph.GenerateNodeId())
	{
		graph.RegisterNode(this, mId);
	}

	DependencyGraph::Node* DependencyGraph::GetNode(NodeId id) noexcept
	{
		return mNodes[id];
	}

	const DependencyGraph::Node* DependencyGraph::GetNode(NodeId id) const noexcept
	{
		return mNodes[id];
	}

	DependencyGraph::EdgeContainer DependencyGraph::GetIncomingEdges(const Node* node) noexcept
	{
		EdgeContainer res;
		res.reserve(mEdges.size());
		const NodeId nodeId = node->GetId();

		std::copy_if(mEdges.begin(), mEdges.end(),
			std::back_insert_iterator<EdgeContainer>(res),
			[nodeId](auto edge) { return edge->to == nodeId; });

		return res;
	}

	DependencyGraph::EdgeContainer DependencyGraph::GetOutgoingEdges(const Node* node) noexcept
	{
		EdgeContainer res;
		res.reserve(mEdges.size());
		const NodeId nodeId = node->GetId();

		std::copy_if(mEdges.begin(), mEdges.end(),
			std::back_insert_iterator<EdgeContainer>(res),
			[nodeId](auto edge) { return edge->from == nodeId; });

		return res;
	}

	void DependencyGraph::Cull() noexcept
	{
		for (auto& edge : mEdges)
		{
			Node* node = GetNode(edge->from);
			++node->mRefCount;
		}

		NodeContainer stack;
		stack.reserve(mNodes.size());
		std::copy_if(mNodes.begin(), mNodes.end(),
			std::back_insert_iterator<NodeContainer>(stack),
			[](auto node) { return node->mRefCount == 0; });

		while (!stack.empty())
		{
			Node* node = stack.back();
			stack.pop_back();
			EdgeContainer incoming = GetIncomingEdges(node);
			for (auto& edge : incoming)
			{
				Node* from = GetNode(edge->from);
				if (--from->mRefCount == 0)
				{
					stack.push_back(from);
				}
			}
		}
	}

	bool DependencyGraph::IsEdgeValid(const Edge* edge) const noexcept
	{
		const Node* from = GetNode(edge->from);
		const Node* to = GetNode(edge->to);
		return !from->IsCulled() && !to->IsCulled();
	}

	bool DependencyGraph::IsAcyclic() const noexcept
	{
#ifdef NDEBUG
		return true;
#else
		return _IsAcyclic();
#endif // !NDEBUG
	}

	bool DependencyGraph::_IsAcyclic() const noexcept
	{
#ifdef NDEBUG
		return true;
#else
		// TODO: 检查是否有环
		return true;
#endif // NDEBUG
	}

	uint32_t DependencyGraph::GenerateNodeId() noexcept
	{
		return mNodes.size();
	}

	void DependencyGraph::RegisterNode(Node* node, NodeId id) noexcept
	{
		assert(id == mNodes.size());
		mNodes.push_back(node);
	}

	void DependencyGraph::Link(Edge* edge) noexcept
	{
		mEdges.push_back(edge);
	}

}
