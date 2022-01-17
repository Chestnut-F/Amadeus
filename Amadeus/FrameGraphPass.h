#pragma once
#include "Prerequisites.h"

namespace Amadeus
{
	class FrameGraph;
	class FrameGraphBuilder;
	class FrameGraphNode;

	class FrameGraphPass
	{
	public:
		virtual void Setup(FrameGraph&, FrameGraphBuilder&, FrameGraphNode*) = 0;

		virtual bool Execute(SharedPtr<DeviceResources>, SharedPtr<DescriptorManager>, SharedPtr<DescriptorCache>) = 0;

	protected:
		ComPtr<ID3D12RootSignature> mRootSignature;
		ComPtr<ID3D12PipelineState> mPipelineState;
		Vector<ComPtr<ID3D12GraphicsCommandList>> mCommandLists;
	};
}