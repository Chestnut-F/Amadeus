#pragma once
#include "Prerequisites.h"
#include "FrameGraphPass.h"

namespace Amadeus
{
	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT4 color;
	};

	const float m_aspectRatio = 1280.0f / 720.0f;

	class FinalPass
		: public FrameGraphPass
	{
	public:
		FinalPass(SharedPtr<DeviceResources> device);

		void Setup() override;

		void Execute(SharedPtr<DeviceResources> device, SharedPtr<DescriptorCache> descriptorCache, SharedPtr<RenderSystem> renderer) override;

	private:
		ComPtr<ID3D12Resource> m_vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	};
}