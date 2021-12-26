#pragma once
#include "Work.h"

namespace Amadeus
{
	class WorkQueue
	{
	private:
		mutable std::mutex mLock;
		mutable std::condition_variable mCondition;
		mutable std::vector<WorkBase*> mWorksToExecute;

		static constexpr uint32_t EXIT_REQUESTED = 0x1;
		UINT mExitRequested = 0;

	public:
		WorkQueue() {}

		~WorkQueue() {}

		std::vector<WorkBase*> WaitForCommands() const
		{
			std::unique_lock<std::mutex> lock(mLock);

			while (mWorksToExecute.empty() && !mExitRequested)
			{
				mCondition.wait(lock);
			}

			return std::move(mWorksToExecute);
		}

		template<typename... ARGS, typename Execute>
		void Submit(Execute&& execute, ARGS&& ... args)
		{
			static_assert(sizeof(Execute) < 1024, "Execute() lambda is capturing too much data.");

			std::unique_lock<std::mutex> lock(mLock);

			Work<ARGS...>* work = new Work<ARGS...>(std::forward<Execute>(execute), std::forward<ARGS>(args)...);

			mWorksToExecute.emplace_back(static_cast<WorkBase*>(work));
		}

		void Flush()
		{
			std::unique_lock<std::mutex> lock(mLock);

			mCondition.notify_one();
		}

		void RequestExit()
		{
			std::lock_guard<std::mutex> lock(mLock);

			mExitRequested = EXIT_REQUESTED;

			mCondition.notify_one();
		}

		bool ExitRequested()
		{
			std::lock_guard<std::mutex> lock(mLock);

			return static_cast<bool>(mExitRequested);
		}
	};

}