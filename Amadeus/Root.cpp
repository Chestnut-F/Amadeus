#include "pch.h"
#include "Root.h"
#include "Common/DeviceResources.h"
#include "FrameGraph.h"
#include "RenderSystem.h"
#include "ResourceManagers.h"
#include "GltfLoader.h"

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
		mDescriptorManager.reset(new DescriptorManager(mDeviceResources));
		mDescriptorCache.reset(new DescriptorCache(mDeviceResources));

		size_t jobs = max(std::thread::hardware_concurrency() - 1, 0);
		mRenderer.reset(new RenderSystem(mDeviceResources, jobs));

		ProgramManager& programManager = ProgramManager::Instance();
		programManager.Init();

		mFrameGraph.reset(new FrameGraph());

		mFrameGraph->AddPass("ShadowPass", mDeviceResources);
		mFrameGraph->AddPass("GBufferPass", mDeviceResources);
		mFrameGraph->AddPass("FinalPass", mDeviceResources);

		Registry& registry = Registry::instance();
		registry.regis();

		Load();
		Upload();
	}

	void Root::PreRender()
	{
		mStepTimer->Tick([]() {});
		CameraManager::Instance().PreRender(mStepTimer->GetElapsedSeconds());
		LightManager::Instance().PreRender(mStepTimer->GetElapsedSeconds());
	}

	void Root::Render()
	{
		CameraManager::Instance().Render();
		LightManager::Instance().Render();

		mFrameGraph->Setup();

		mFrameGraph->Compile(mDeviceResources, mDescriptorCache);

		mFrameGraph->Execute(mDeviceResources, mDescriptorManager, mDescriptorCache, mRenderer);
	}

	void Root::PostRender()
	{
		mDescriptorCache->Reset(mDeviceResources);
		CameraManager::Instance().PostRender();
		LightManager::Instance().PostRender();
	}

	void Root::Destroy()
	{
		mRenderer->Destroy();
		mDeviceResources->WaitForGpu();
		mFrameGraph->Destroy();

		MaterialManager::Instance().Destroy();

		MeshManager::Instance().Destroy();

		TextureManager::Instance().Destroy();

		LightManager::Instance().Destroy();

		CameraManager::Instance().Destroy();

		mDescriptorCache->Destroy();
		mDescriptorManager->Destroy();

		mDeviceResources = nullptr;

#if defined(_DEBUG)
		{
			ComPtr<IDXGIDebug1> dxgiDebug;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
			{
				dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			}
		}
#endif
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

	void Root::Load()
	{
		CameraManager& cameraManager = CameraManager::Instance();
		cameraManager.Init();

		LightManager& lightMananger = LightManager::Instance();
		lightMananger.Init();

		Gltf::LoadGltf(L"Sponza", mDeviceResources, mDescriptorManager);

		TextureManager::Instance().LoadFromFile(TEXTURE_EMPTY_ID, mDeviceResources, mDescriptorManager);

		MeshManager& meshManager = MeshManager::Instance();
		meshManager.Init();
		auto& boundary = meshManager.GetBoundary();
		XMVECTOR centralLocation = meshManager.GetCentralLocation();

		cameraManager.Create({ boundary.xMax * 2.f, boundary.yMax * 2.f, boundary.zMax * 2.f }, centralLocation);
		lightMananger.Create({ boundary.xMax * 1.0f, abs(boundary.yMax) * 2.f, boundary.zMin * 1.f }, centralLocation);
	}

	void Root::Upload()
	{
		CameraManager::Instance().Upload(mDeviceResources);

		LightManager::Instance().Upload(mDeviceResources);

		TextureManager::Instance().UploadAll(mDeviceResources, mRenderer);

		MeshManager::Instance().UploadAll(mDeviceResources, mRenderer);
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

