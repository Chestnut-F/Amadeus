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
		FrameGraphPass(bool target) : bTarget(target) {}

		virtual void Setup(FrameGraph&, FrameGraphBuilder&, FrameGraphNode*) = 0;

		virtual void RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache) = 0;

		virtual bool Execute(SharedPtr<DeviceResources>, SharedPtr<DescriptorManager>, SharedPtr<DescriptorCache>) = 0;

		virtual void Destroy()
		{
			mCommandLists.clear();
		}

		bool IsTarget() { return bTarget; }

	protected:
		ComPtr<ID3D12RootSignature> mRootSignature;
		ComPtr<ID3D12PipelineState> mPipelineState;
		Vector<ComPtr<ID3D12GraphicsCommandList>> mCommandLists;

		bool bTarget;
	};
}