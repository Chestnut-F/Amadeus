#pragma once
#include "Prerequisites.h"
#include "Light.h"

namespace Amadeus
{
	static constexpr int DEFAULT_LIGHT = 0;

	class LightManager : public Observer
	{
	public:
		LightManager(const LightManager&) = delete;
		LightManager& operator=(const LightManager&) = delete;

		static LightManager& Instance()
		{
			static LightManager* instance = new LightManager();
			return *instance;
		}

		void Init();
		void PreRender(float elapsedSeconds);
		void Render();
		void PostRender();
		void Destroy();

		UINT64 Create();

		Light& GetDefaultCamera() { return *mLightList[DEFAULT_LIGHT]; }

	public:
		LightManager() {}

		typedef Vector<Light*> LightList;
		LightList mLightList;
		UINT64 mSize;
	};
}