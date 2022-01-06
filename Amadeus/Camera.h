#pragma once
#include "Prerequisites.h"

namespace Amadeus
{
	class Camera
	{
	public:
		struct CameraConstantBuffer
		{
			XMFLOAT4X4 view;
			XMFLOAT4X4 projection;
			XMFLOAT3 eyePosWorld;
			float nearPlane;
			float farPlane;
			float padding[27];
		};
		static_assert((sizeof(CameraConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

	public:
		Camera(XMVECTOR position, XMVECTOR lookAt = { 0.0f, 0.0f, 0.0f }, XMVECTOR up = { 0.0f, 1.0f, 0.0f }, 
			float fov = XM_PI / 3.0f, float aspectRatio = 16.0f / 9.0f, float nearPlane = 1.0f, float farPlane = 1000.0f);

		void Upload(SharedPtr<DeviceResources> device);

		void Update(XMVECTOR position, XMVECTOR lookAt, XMVECTOR up);

		XMVECTOR GetPosition();
		XMVECTOR GetLookAtPosition();
		XMVECTOR GetLookDirection();
		XMVECTOR GetUpirection();
		float GetRadius();
		XMMATRIX GetViewMatrix();
		XMMATRIX GetProjectionMatrix();
		XMMATRIX GetTransformMatrix();
		XMMATRIX GetTransformMatrixWithoutTranslation();
		float GetNearPlane();
		float GetFarPlane();

		D3D12_CONSTANT_BUFFER_VIEW_DESC GetCbvDesc(SharedPtr<DeviceResources> device);

	private:
		XMFLOAT3 mPosition;
		XMFLOAT3 mLookAtPosition;

		XMFLOAT3 mRightDirection;	// local x
		XMFLOAT3 mUpDirection;		// local y
		XMFLOAT3 mLookDirection;	// local z

		float mRadius;
		float mFov;
		float mAspectRatio;
		float mNearPlane;
		float mFarPlane;

		ComPtr<ID3D12Resource> mCameraConstants;
		CameraConstantBuffer mCameraConstantBuffer;
		UINT8* pCameraCbvDataBegin;
		const UINT mCameraConstantBufferSize = sizeof(CameraConstantBuffer);
	};
}