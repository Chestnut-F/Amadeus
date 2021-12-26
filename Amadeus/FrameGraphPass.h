#pragma once
#include "Prerequisites.h"

namespace Amadeus
{
	class FrameGraphPass
	{
	public:
		virtual void Setup() = 0;

		virtual void Execute(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache, SharedPtr<RenderSystem> renderer) = 0;

	protected:
		ComPtr<ID3D12RootSignature> mRootSignature;
		ComPtr<ID3D12PipelineState> mPipelineState;
		ComPtr<ID3D12GraphicsCommandList> mCommandList;
	};
}