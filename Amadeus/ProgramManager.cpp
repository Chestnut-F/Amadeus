#include "pch.h"
#include "ProgramManager.h"
#include <io.h>

namespace Amadeus
{
	void ProgramManager::Init()
	{
		String shadersPath = WString2String(GetAssetFullPath(L"Shaders")).append("\\*.cso");
		struct _finddata_t c_file;
		intptr_t hFile;

		if ((hFile = _findfirst(shadersPath.c_str(), &c_file)) != -1L)
		{
			do {
				mProgramMap[c_file.name] = new Program(c_file.name);
			} while (_findnext(hFile, &c_file) == 0);
			_findclose(hFile);
		}
	}

	void ProgramManager::Load(const String& name, ShaderType type)
	{
		auto programIter = mProgramMap.find(name);

		if (programIter != mProgramMap.end())
		{
			programIter->second->Load(type);
		}
		else
		{
			throw Exception("Invalid shader.");
		}
	}

	void ProgramManager::Unload(const String& name)
	{
		auto programIter = mProgramMap.find(name);

		if (programIter != mProgramMap.end())
		{
			programIter->second->Unload();
		}
	}

	void ProgramManager::UnloadAll()
	{
		for (auto& program : mProgramMap)
		{
			program.second->Unload();
		}
	}

	ID3DBlob* ProgramManager::Get(const String& name)
	{
		auto programIter = mProgramMap.find(name);
		if (programIter != mProgramMap.end())
		{
			if (programIter->second->Loaded())
			{
				return programIter->second->Get();
			}
			else
			{
				Load(name);
				return programIter->second->Get();
			}
		}
		else
		{
			return nullptr;
		}
	}

	uint64_t ProgramManager::GetBufferSize(const String& name)
	{
		auto programIter = mProgramMap.find(name);
		if (programIter != mProgramMap.end())
		{
			if (programIter->second->Loaded())
			{
				return programIter->second->GetBufferSize();
			}
			else
			{
				Load(name);
				return programIter->second->GetBufferSize();
			}
		}
		else
		{
			return 0;
		}
	}
}