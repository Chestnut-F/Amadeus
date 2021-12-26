#pragma once

namespace Amadeus
{
	class WorkBase
	{
	public:
		// ����std::invoke()ʵ�ֵ�ִ��
		virtual void invoke() = 0;

		// ����std::apply()ʵ�ֵ�ִ��
		virtual void execute() = 0;
	};

	template<typename... ARGS>
	class Work : public WorkBase
	{
	public:
		using Execute = std::function<void(ARGS...)>;
		using SavedParameters = std::tuple<std::remove_reference_t<ARGS>...>;

		Work(Execute&& exectue, ARGS&& ... args)
			: mExectue(std::forward<Execute>(exectue))
			, mArgs(std::forward<ARGS>(args)...)
		{
		}

		template<std::size_t... I>
		void invoke(std::index_sequence<I...>)
		{
			std::invoke(std::forward<Execute>(mExectue), std::get<I>(std::forward<SavedParameters>(mArgs))...);
		}

		void invoke() override
		{
			invoke(std::make_index_sequence<std::tuple_size<std::remove_reference_t<SavedParameters>>::value>{});
		}

		void execute() override
		{
			std::apply(std::forward<Execute>(mExectue), std::forward<SavedParameters>(mArgs));
		}

	private:
		Execute mExectue;
		SavedParameters mArgs;
	};
}