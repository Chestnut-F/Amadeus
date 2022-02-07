#pragma once
#include "Exports.h"

namespace Amadeus
{
	class StepTimer;
	class DeviceResources;
	class DescriptorManager;
	class DescriptorCache;
	class FrameGraph;
	class RenderSystem;

	class _AmadeusExport Root
	{
	public:
		Root(const wchar_t* title, unsigned int width, unsigned int height, HWND hwnd);
		~Root();

		std::shared_ptr<DeviceResources> GetDeviceResources();

		void Init();
		void PreRender();
		void Render();
		void PostRender();
		void Destroy();

		void OnMouseWheel(INT8 zDelta);
		void OnMouseMove(INT x, INT y);
		void OnButtonDown(INT button, INT x, INT y);
		void OnButtonUp();

	private:
		void Load();
		void Upload();
		void PreCompute();

		std::wstring mTitle;

		UINT32 mWidth;
		UINT32 mHeight;
		float mAspectRatio; 
		float mFov;

		HWND mHwnd;

		std::shared_ptr<DeviceResources> mDeviceResources;

		std::shared_ptr<DescriptorManager> mDescriptorManager;

		std::shared_ptr<DescriptorCache> mDescriptorCache;

		std::shared_ptr<RenderSystem> mRenderer;

		std::unique_ptr<FrameGraph> mFrameGraph;

		std::unique_ptr<StepTimer> mStepTimer;
	};
}