#pragma once
#include "Enums.h"
#include "Subject.h"
#include "Registry.h"
#include "Observer.h"

namespace Amadeus
{
	using String = std::string;
	using WString = std::wstring;
	using RuntimeError = std::runtime_error;
	using Exception = std::exception;
	using OutOfRange = std::out_of_range;
	template<typename T> using Function = std::function<T>;
	template<typename T> using NumericLimits = std::numeric_limits<T>;
	template<typename T> using Vector = std::vector<T>;
	template<typename K, typename V> using Pair = std::pair<K, V>;
	template<typename K, typename V> using Map = std::map<K, V>;
	template<typename T> using UniquePtr = std::unique_ptr<T>;
	template<typename T> using SharedPtr = std::shared_ptr<T>;
	using Thread = std::thread;
	using Mutex = std::mutex;
	using Condition = std::condition_variable;
	template<typename T> using UniqueLock = std::unique_lock<T>;
	template<typename T> using LockGuard = std::lock_guard<T>;
	template<typename T> using Optional = std::optional<T>;

	using namespace DirectX;
	using namespace Microsoft::WRL;

	static constexpr UINT FrameCount = 3;
	static constexpr UINT Width = 3;
	static constexpr UINT Height = 3;

	class DeviceResources;
	class FrameGraph;
	class FrameGraphPass;
	class RenderSystem;
}