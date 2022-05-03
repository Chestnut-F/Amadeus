#include "pch.h"
#include "CameraManager.h"
#include "RenderSystem.h"

namespace Amadeus
{
	void CameraManager::Init()
	{
		listen<MouseWheel>("MouseWheel",
			[&](MouseWheel params)
		{
			if (input.wheel || input.lButton || input.rButton)
				return;

			input.wheel = true;
			input.zDelta = params.zDelta;
		});

		listen<MouseMove>("MouseMove",
			[&](MouseMove params)
		{
			input.x = params.x;
			input.y = params.y;
		});

		listen<MouseButtonDown>("MouseButtonDown",
			[&](MouseButtonDown params)
		{
			if (input.wheel || input.lButton || input.rButton)
				return;

			input.x = params.x;
			input.y = params.y;
			input.lastX = params.x;
			input.lastY = params.y;
			if (params.button & 0x01)
			{
				input.lButton = true;
			}
			if (params.button & 0x02)
			{
				input.rButton = true;
			}
		});

		listen<MouseButtonUp>("MouseButtonUp",
			[&](MouseButtonUp params)
		{
			input.lButton = false;
			input.rButton = false;
		});

		listen<GBufferRender>("GBufferRender",
			[&](GBufferRender params)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = GetDefaultCamera().GetCbvDesc(params.device);
			CD3DX12_GPU_DESCRIPTOR_HANDLE cameraConstantsHandle = params.descriptorCache->AppendCbvCache(params.device, cbvDesc);
			params.commandList->SetGraphicsRootConstantBufferView(COMMON_CAMERA_ROOT_CBV_INDEX, cbvDesc.BufferLocation);
		});

		listen<ZPreRender>("ZPreRender",
			[&](ZPreRender params)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = GetDefaultCamera().GetCbvDesc(params.device);
			CD3DX12_GPU_DESCRIPTOR_HANDLE cameraConstantsHandle = params.descriptorCache->AppendCbvCache(params.device, cbvDesc);
			params.commandList->SetGraphicsRootConstantBufferView(COMMON_CAMERA_ROOT_CBV_INDEX, cbvDesc.BufferLocation);
		});

		listen<SSAORender>("SSAORender",
			[&](SSAORender params)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = GetDefaultCamera().GetCbvDesc(params.device);
			CD3DX12_GPU_DESCRIPTOR_HANDLE cameraConstantsHandle = params.descriptorCache->AppendCbvCache(params.device, cbvDesc);
			params.commandList->SetGraphicsRootConstantBufferView(COMMON_CAMERA_ROOT_CBV_INDEX, cbvDesc.BufferLocation);
		});

		listen<TAARender>("TAARender",
			[&](TAARender params)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = GetDefaultCamera().GetCbvDesc(params.device);
			CD3DX12_GPU_DESCRIPTOR_HANDLE cameraConstantsHandle = params.descriptorCache->AppendCbvCache(params.device, cbvDesc);
			params.commandList->SetGraphicsRootConstantBufferView(COMMON_CAMERA_ROOT_CBV_INDEX, cbvDesc.BufferLocation);
		});

		listen<SkyboxRender>("SkyboxRender",
			[&](SkyboxRender params)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = GetDefaultCamera().GetCbvDesc(params.device);
			CD3DX12_GPU_DESCRIPTOR_HANDLE cameraConstantsHandle = params.descriptorCache->AppendCbvCache(params.device, cbvDesc);
			params.commandList->SetGraphicsRootConstantBufferView(COMMON_CAMERA_ROOT_CBV_INDEX, cbvDesc.BufferLocation);
		});

		listen<GBufferTransparentRender>("GBufferTransparentRender",
			[&](GBufferTransparentRender params)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = GetDefaultCamera().GetCbvDesc(params.device);
			CD3DX12_GPU_DESCRIPTOR_HANDLE cameraConstantsHandle = params.descriptorCache->AppendCbvCache(params.device, cbvDesc);
			params.commandList->SetGraphicsRootConstantBufferView(COMMON_CAMERA_ROOT_CBV_INDEX, cbvDesc.BufferLocation);
		});
	}

	void CameraManager::PreRender(float elapsedSeconds)
	{
		auto camera = mCameraList[DEFAULT_CAMERA];

		if (input.wheel)
		{
			XMVECTOR pos = camera->GetPosition();
			XMVECTOR moveDir = camera->GetLookDirection();
			XMVECTOR lookAt = camera->GetLookAtPosition();
			float radius = camera->GetRadius();

			float dis = (float)input.zDelta * FORWARD_SPEED * elapsedSeconds;
			dis = radius < dis ? 0.0f : dis;
			pos += dis * moveDir;
			XMStoreFloat(&radius, XMVector3Length(lookAt - pos));
			pos = radius > MAX_RADIUS ? (-moveDir * MAX_RADIUS) + lookAt : pos;

			camera->Update(pos, lookAt, camera->GetUpirection());
			return;
		}

		if (input.lButton)
		{
			XMVECTOR moveDir = { input.lastX - input.x, input.y - input.lastY, 0.0f };
			XMMATRIX R = camera->GetTransformMatrixWithoutTranslation();
			moveDir = XMVector3TransformCoord(moveDir, R);
			
			float dis = 0.0f;
			XMStoreFloat(&dis, XMVector3Length(moveDir));
			dis = dis * ROTATION_SPEED * elapsedSeconds;

			XMVECTOR pos = camera->GetPosition();
			float radius = camera->GetRadius();
			XMVECTOR lookAt = camera->GetLookAtPosition();

			pos += dis * moveDir;
			pos = lookAt + radius * XMVector3Normalize(pos - lookAt);

			camera->Update(pos, lookAt, camera->GetUpirection());
			return;
		}

		if (input.rButton)
		{
			XMVECTOR moveDir = { input.lastX - input.x, input.y - input.lastY, 0.0f };
			XMMATRIX R = camera->GetTransformMatrixWithoutTranslation();
			moveDir = XMVector3Transform(moveDir, R);

			float dis = 0.0f;
			XMStoreFloat(&dis, XMVector3Length(moveDir));
			dis = dis * LATERAL_SPEED * elapsedSeconds;

			XMVECTOR pos = camera->GetPosition();
			float radius = camera->GetRadius();
			XMVECTOR lookAt = camera->GetLookAtPosition();

			pos += dis * moveDir;
			lookAt += dis * moveDir;

			camera->Update(pos, lookAt, camera->GetUpirection());
			return;
		}

		camera->Update(camera->GetPosition(), { 0.0f, 0.0f, 0.0f }, camera->GetUpirection());
	}

	void CameraManager::Render()
	{

	}

	void CameraManager::PostRender()
	{
		input.wheel = false;
		input.zDelta = 0;
		input.lastX = input.x;
		input.lastY = input.y;
	}

	void CameraManager::Destroy()
	{
		for (auto& camera : mCameraList)
		{
			camera->Destroy();
		}
	}

	UINT64 CameraManager::Create( XMVECTOR position, XMVECTOR lookAtPosition, XMVECTOR upDirection,
		float fov, float aspectRatio, float nearPlane, float farPlane)
	{
		UINT64 res = mSize;
		Camera* camera = new Camera(position, lookAtPosition, upDirection, fov, aspectRatio, nearPlane, farPlane);
		mCameraList.emplace_back(camera);
		mSize++;
		assert(mSize == mCameraList.size());

		return res;
	}

	void CameraManager::Upload(SharedPtr<DeviceResources> device)
	{
		assert(mSize == 1);

		for (auto camera : mCameraList)
		{
			camera->Upload(device);
		}
	}

}