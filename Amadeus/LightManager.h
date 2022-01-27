#pragma once
#include "Prerequisites.h"
#include "Light.h"

namespace Amadeus
{
	static constexpr int DEFAULT_LIGHT = 0;
	static constexpr int SUN_LIGHT = 0;

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

		UINT64 Create(XMVECTOR pos, XMVECTOR at, XMVECTOR color = { 1.0f, 1.0f, 1.0f }, float intensity = 100000.0f);

		void Upload(SharedPtr<DeviceResources> device);

		Light& GetDefaultLight() { return *mLightList[DEFAULT_LIGHT]; }

		Light& GetSunLight() { return *mLightList[SUN_LIGHT]; }

		float GetNearPlane() { return mLightList[SUN_LIGHT]->mNearPlane; }

		float GetFarPlane() { return mLightList[SUN_LIGHT]->mFarPlane; }

	public:
		LightManager() : mSize(0) {}

		typedef Vector<Light*> LightList;
		LightList mLightList;
		UINT64 mSize;
	};
}