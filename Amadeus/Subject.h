#pragma once

namespace Amadeus
{
	class SubjectBase
	{
	public:
		SubjectBase(const std::string& name) : mName(name) {}
		SubjectBase(std::string&& name) : mName(std::move(name)) {}
		virtual ~SubjectBase() = default;

	protected:
		std::string mName;
	};

	template<typename Params, typename Callback = std::function<void(Params)>>
	class Subject : public SubjectBase
	{
	public:
		Subject(const std::string& name);
		Subject(std::string&& name);
		~Subject();

		void addListener(const std::type_info& listener, Callback&& func);
		void removeListener(const std::type_info& listener);

		void notify(Params params);

	private:
		struct Listener
		{
			Listener(const std::type_info& listener, Callback&& func)
				: listener(listener.name())
				, func(std::move(func))
			{
			}

			std::string listener;
			Callback func;
		};

		std::string paramsInfo;
		std::list<Listener> mListeners;
	};

	template<typename Params, typename Callback>
	inline Subject<Params, Callback>::Subject(const std::string& name)
		: SubjectBase(name)
		, paramsInfo(typeid(Params).name())
	{
	}

	template<typename Params, typename Callback>
	inline Subject<Params, Callback>::Subject(std::string&& name)
		: SubjectBase(std::move(name))
		, paramsInfo(typeid(Params).name())
	{
	}

	template<typename Params, typename Callback>
	inline Subject<Params, Callback>::~Subject()
	{
	}

	template<typename Params, typename Callback>
	inline void Subject<Params, Callback>::addListener(const std::type_info& listener, Callback&& func)
	{
		mListeners.emplace_back(Listener(listener, std::forward<Callback>(func)));
	}

	template<typename Params, typename Callback>
	inline void Subject<Params, Callback>::removeListener(const std::type_info& listener)
	{
		auto end = mListeners.end();

		for (auto iter = mListeners.begin(); iter != end; ++iter)
		{
			if (iter->listener == listener.name())
			{
				mListeners.erase(iter);
			}
		}
	}

	template<typename Params, typename Callback>
	inline void Subject<Params, Callback>::notify(Params params)
	{
		assert(typeid(Params).name() == paramsInfo);

		for (auto& listener : mListeners)
		{
			listener.func(params);
		}
	}
}

