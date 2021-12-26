#pragma once

namespace Amadeus
{
	class Observer
	{
	public:
		virtual ~Observer() = default;

		template<typename Params, typename Callback>
		void listen(const std::string& eventName, Callback&& func);

		template<typename Params>
		void unListen(const std::string& eventName);
	};

	template<typename Params, typename Callback>
	inline void Observer::listen(const std::string& eventName, Callback&& func)
	{
		static_assert(sizeof(Callback) < 1024, "Callback() lambda is capturing too much data.");

		auto* subject = Registry::instance().query<Params>(eventName);

		subject->addListener(typeid(*this), std::forward<Callback>(func));
	}

	template<typename Params>
	inline void Observer::unListen(const std::string& eventName)
	{
		auto* subject = Registry::instance().query<Params>(eventName);

		subject->removeListener(typeid(*this));
	}
}