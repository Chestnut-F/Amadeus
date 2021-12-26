#pragma once
#include "Prerequisites.h"

namespace Amadeus
{
	class RenderSystem
	{
	public:
		RenderSystem();

		void Submit(ID3D12CommandList* commandList);

		void Render(SharedPtr<DeviceResources> device);

		void Destroy();

	private:
		Vector<ID3D12CommandList*> ppCommandLists;
	};
}