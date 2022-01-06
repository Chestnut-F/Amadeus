#pragma once
#include "Prerequisites.h"
#include "Camera.h"

namespace Amadeus
{
	static constexpr int DEFAULT_CAMERA = 0;
	static constexpr float FORWARD_SPEED = 1.0f;
	static constexpr float LATERAL_SPEED = 1.0f;
	static constexpr float ROTATION_SPEED = 1.0f;
	static constexpr float MAX_RADIUS = 800.0f;

	class CameraManager : public Observer
	{
	public:
		CameraManager(const CameraManager&) = delete;
		CameraManager& operator=(const CameraManager&) = delete;

		static CameraManager& Instance()
		{
			static CameraManager* instance = new CameraManager();
			return *instance;
		}

		void Init();
		void PreRender(float elapsedSeconds);
		void Render();
		void PostRender();
		void Destroy();

		UINT64 Create(
			XMVECTOR position, 
			XMVECTOR lookAtPosition = { 0.0f, 0.0f, 0.0f }, 
			XMVECTOR upDirection = { 0.0f, 1.0f, 0.0f }, 
			float fov = XM_PI / 3.0f, 
			float aspectRatio = 16.0f / 9.0f, 
			float nearPlane = 1.0f, 
			float farPlane = 1000.0f);

		void Upload(SharedPtr<DeviceResources> device);

		Camera& GetDefaultCamera() { return *mCameraList[DEFAULT_CAMERA]; }

	private:
		CameraManager() : mSize(0), input{} {}

		struct MouseInput
		{
			INT x;
			INT y;

			INT lastX;
			INT lastY;

			INT zDelta;
			
			bool lButton = false;
			bool rButton = false;
			bool wheel = false;
		} input;

		typedef Vector<Camera*> CameraList;
		CameraList mCameraList;
		UINT64 mSize;
	};
}