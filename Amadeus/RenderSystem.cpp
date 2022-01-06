#include "pch.h"
#include "RenderSystem.h"

namespace Amadeus
{
	RenderSystem::RenderSystem(SharedPtr<DeviceResources> device, size_t jobs)
		: mRenderThread(1, L"RenderThread")
		, mJobSystem(jobs, L"JobThread")
		, mJobs(jobs)
	{
	}

	void RenderSystem::Render(SharedPtr<DeviceResources> device)
	{
		device->Present();
	}

	void RenderSystem::Upload(SharedPtr<DeviceResources> device)
	{
		device->WaitForGpu();
	}

	void RenderSystem::Destroy()
	{
	}
}