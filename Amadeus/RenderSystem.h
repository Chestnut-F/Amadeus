#pragma once
#include "Prerequisites.h"

namespace Amadeus
{
	class RenderSystem
	{
	public:
		RenderSystem(SharedPtr<DeviceResources> device, size_t jobs = 0);

		template<class F, class... Args>
		auto Execute(F&& f, Args&&... args)
			->Future<typename std::invoke_result<F, Args...>::type>;

		template<class F, class... Args>
		auto Submit(F&& f, Args&&... args)
			->Future<typename std::invoke_result<F, Args...>::type>;

		void Render(SharedPtr<DeviceResources> device);

		void Upload(SharedPtr<DeviceResources> device);

		void Destroy();

		size_t GetJobs() { return mJobs; }

	private:
		ThreadPool mRenderThread;
		ThreadPool mJobSystem;

		size_t mJobs;
	};

	template<class F, class ...Args>
	inline auto RenderSystem::Execute(F&& f, Args && ...args)
		->Future<typename std::invoke_result<F, Args...>::type>
	{
		return mRenderThread.enqueue(std::forward<F>(f), std::forward<Args>(args)...);
	}

	template<class F, class ...Args>
	inline auto RenderSystem::Submit(F&& f, Args && ...args) -> Future<typename std::invoke_result<F, Args ...>::type>
	{
		return mJobSystem.enqueue(std::forward<F>(f), std::forward<Args>(args)...);
	}
}