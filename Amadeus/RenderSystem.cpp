#include "pch.h"
#include "RenderSystem.h"

namespace Amadeus
{
	RenderSystem::RenderSystem()
	{
	}

	void RenderSystem::Submit(ID3D12CommandList* commandList)
	{
		ppCommandLists.emplace_back(commandList);
	}

	void RenderSystem::Render(SharedPtr<DeviceResources> device)
	{
		device->GetCommandQueue()->ExecuteCommandLists(ppCommandLists.size(), ppCommandLists.data());

		device->Present();

		ppCommandLists.clear();
	}

	void RenderSystem::Destroy()
	{
	}
}