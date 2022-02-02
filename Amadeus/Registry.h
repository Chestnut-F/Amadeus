#pragma once

namespace Amadeus
{
	class Registry
	{
	public:
		Registry(const Registry&) = delete;
		Registry& operator=(const Registry&) = delete;

		static Registry& instance()
		{
			static Registry* instance = new Registry();
			return *instance;
		}

		void regis();
		template<typename Params>
		void regis(const std::string& eventName);
		template<typename Params>
		Subject<Params>* query(const std::string& eventName);

	private:
		Registry() {}
		std::map<std::string, SubjectBase*> registry;
	};

	template<typename Params>
	inline Subject<Params>* Registry::query(const std::string& eventName)
	{
		if (registry.find(eventName) != registry.end())
		{
			return dynamic_cast<Subject<Params>*>(registry[eventName]);
		}
		else
		{
			return nullptr;
		}
	}

	template<typename Params>
	inline void Registry::regis(const std::string& eventName)
	{
		if (registry.find(eventName) != registry.end())
		{
			throw std::exception("Repeat registration event.");
		}

		Subject<Params>* subject = new Subject<Params>(eventName);

		registry[eventName] = static_cast<SubjectBase*>(subject);
	}
}

// ×¢²áÊÂ¼þ
namespace Amadeus
{
	struct MouseWheel
	{
		int32_t zDelta;
	};

	struct MouseMove
	{
		int32_t x;
		int32_t y;
	};

	struct MouseButtonDown
	{
		int32_t button;
		int32_t x;
		int32_t y;
	};

	struct MouseButtonUp
	{
	};

	struct GBufferRender
	{
		std::shared_ptr<DeviceResources> device;
		std::shared_ptr<DescriptorCache> descriptorCache;
		ID3D12GraphicsCommandList* commandList;
	};

	struct ShadowMapRender
	{
		std::shared_ptr<DeviceResources> device;
		std::shared_ptr<DescriptorCache> descriptorCache;
		ID3D12GraphicsCommandList* commandList;
	};

	struct ZPreRender
	{
		std::shared_ptr<DeviceResources> device;
		std::shared_ptr<DescriptorCache> descriptorCache;
		ID3D12GraphicsCommandList* commandList;
	};

	struct SSAORender
	{
		std::shared_ptr<DeviceResources> device;
		std::shared_ptr<DescriptorCache> descriptorCache;
		ID3D12GraphicsCommandList* commandList;
	};

	inline void Registry::regis()
	{
		regis<MouseWheel>("MouseWheel");
		regis<MouseMove>("MouseMove");
		regis<MouseButtonDown>("MouseButtonDown");
		regis<MouseButtonUp>("MouseButtonUp");
		regis<GBufferRender>("GBufferRender");
		regis<ShadowMapRender>("ShadowMapRender");
		regis<ZPreRender>("ZPreRender");
		regis<SSAORender>("SSAORender");
	}
}