#include "pch.h"
#include "Root.h"
#include "Common/DeviceResources.h"
#include "FrameGraph.h"
#include "RenderSystem.h"
#include "CameraManager.h"
#include "ProgramManager.h"

namespace Amadeus
{
	Root::Root(const wchar_t* title, unsigned int width, unsigned int height, HWND hwnd)
		: mTitle(title)
		, mWidth(width)
		, mHeight(height)
		, mHwnd(hwnd)
	{
		mAspectRatio = static_cast<float>(width) / static_cast<float>(height);
		mFov = XM_PI / 3.0f;

		mStepTimer.reset(new StepTimer());
	}

	Root::~Root()
	{
	}

	void Root::Init()
	{
		GetDeviceResources();

		mDescriptorCache.reset(new DescriptorCache(mDeviceResources));

		mRenderer.reset(new RenderSystem());

		ProgramManager& programManager = ProgramManager::Instance();
		programManager.Init();

		mFrameGraph.reset(new FrameGraph());

		mFrameGraph->AddPass("FinalPass", mDeviceResources);

		Registry& registry = Registry::instance();
		registry.regis();

		CameraManager& cameraManager = CameraManager::Instance();
		cameraManager.Init();
		cameraManager.Create(mDeviceResources, { 30.0f, 0.0f, 0.0f });
	}

	void Root::PreRender()
	{
		mStepTimer->Tick([]() {});
		CameraManager::Instance().PreRender(mStepTimer->GetElapsedSeconds());
	}

	void Root::Render()
	{
		CameraManager::Instance().Render();
		mFrameGraph->Execute(mDeviceResources, mDescriptorCache, mRenderer);
	}

	void Root::PostRender()
	{
		mDescriptorCache->Reset();
		CameraManager::Instance().PostRender();
	}

	void Root::Destroy()
	{
		mRenderer->Destroy();
	}

	void Root::OnMouseWheel(INT8 zDelta)
	{
		MouseWheel params = {};
		params.zDelta = zDelta;

		Subject<MouseWheel>* mouseWheel = Registry::instance().query<MouseWheel>("MouseWheel");
		if (mouseWheel)
		{
			mouseWheel->notify(params);
		}
	}

	void Root::OnMouseMove(INT x, INT y)
	{
		MouseMove params = {};
		params.x = x;
		params.y = y;

		Subject<MouseMove>* mouseMove = Registry::instance().query<MouseMove>("MouseMove");
		if (mouseMove)
		{
			mouseMove->notify(params);
		}
	}

	void Root::OnButtonDown(INT button, INT x, INT y)
	{
		MouseButtonDown params = {};
		params.button = button;
		params.x = x;
		params.y = y;

		Subject<MouseButtonDown>* mouseButtonDown = Registry::instance().query<MouseButtonDown>("MouseButtonDown");
		if (mouseButtonDown)
		{
			mouseButtonDown->notify(params);
		}
	}

	void Root::OnButtonUp()
	{
		MouseButtonUp params = {};

		Subject<MouseButtonUp>* mouseButtonUp = Registry::instance().query<MouseButtonUp>("MouseButtonUp");
		if (mouseButtonUp)
		{
			mouseButtonUp->notify(params);
		}
	}

	std::shared_ptr<DeviceResources> Root::GetDeviceResources()
	{
		if (mDeviceResources != nullptr && mDeviceResources->IsDeviceRemoved())
		{
			// 必须先释放对现有 D3D 设备的所有引用，然后才能
			// 创建新设备。

			mDeviceResources = nullptr;

#if defined(_DEBUG)
			ComPtr<IDXGIDebug1> dxgiDebug;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
			{
				dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
			}
#endif
		}

		if (mDeviceResources == nullptr)
		{
			mDeviceResources = std::make_shared<DeviceResources>();
			mDeviceResources->SetWindow(mHwnd, mWidth, mHeight);
		}
		return mDeviceResources;
	}
}

