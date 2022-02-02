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
		FrameGraphPass(bool target) : bTarget(target), bUploaded(false) {}

		virtual bool PreCompute(SharedPtr<DeviceResources> device, ID3D12GraphicsCommandList* commandList);

		virtual void PostPreCompute();

		virtual void Setup(FrameGraph&, FrameGraphBuilder&, FrameGraphNode*) = 0;

		virtual void RegisterResource(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache) = 0;

		virtual bool Execute(SharedPtr<DeviceResources> device, 
			SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache);

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
		bool bUploaded;
	};
}