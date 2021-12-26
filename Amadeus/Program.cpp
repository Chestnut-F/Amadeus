#include "pch.h"
#include "Program.h"

namespace Amadeus
{
	Program::Program(const String& name)
		: mName(name)
	{
		mPath = GetAssetFullPath(L"Shaders\\" + String2WString(mName));
	}

	Program::Program(String&& name)
		: mName(std::move(name))
	{
		mPath = GetAssetFullPath(L"Shaders\\" + String2WString(mName));
	}

	void Program::Load(ShaderType type)
	{
		if (bLoaded)
		{
			return;
		}

		ThrowIfFailed(D3DReadFileToBlob(mPath.c_str(), &mShader));

		bLoaded = true;
	}

	void Program::Unload()
	{
		if (!bLoaded)
		{
			return;
		}
		mShader->Release();
		bLoaded = false;
	}

	ID3DBlob* Program::Get()
	{
		if (bLoaded)
		{
			return mShader.Get();
		}
		else
		{
			return nullptr;
		}
	}

	uint64_t Program::GetBufferSize()
	{
		if (bLoaded)
		{
			return mShader->GetBufferSize();
		}
		else
		{
			return 0;
		}
	}
}