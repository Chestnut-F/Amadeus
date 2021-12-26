#pragma once
#include "Prerequisites.h"
#include "Program.h"

namespace Amadeus
{
	class ProgramManager
	{
	public:
		ProgramManager(const ProgramManager&) = delete;
		ProgramManager& operator=(const ProgramManager&) = delete;

		static ProgramManager& Instance()
		{
			static ProgramManager* instance = new ProgramManager();
			return *instance;
		}

		void Init();

		void Load(const String& name, ShaderType type = ShaderType::TypeLess);

		void Unload(const String& name);

		void UnloadAll();

		ID3DBlob* Get(const String& name);

		uint64_t GetBufferSize(const String& name);

	private:
		ProgramManager() {}

		typedef Map<String, Program*> ProgramMap;
		ProgramMap mProgramMap;
	};
}