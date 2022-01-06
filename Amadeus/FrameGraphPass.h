#pragma once
#include "Prerequisites.h"

namespace Amadeus
{
	class FrameGraphPass
	{
	public:
		virtual void Setup() = 0;

		virtual bool Execute(
			SharedPtr<DeviceResources> device, SharedPtr<DescriptorManager> descriptorManager, SharedPtr<DescriptorCache> descriptorCache) = 0;

	protected:
		ComPtr<ID3D12RootSignature> mRootSignature;
		ComPtr<ID3D12PipelineState> mPipelineState;
		Vector<ComPtr<ID3D12GraphicsCommandList>> mCommandLists;
	};
}