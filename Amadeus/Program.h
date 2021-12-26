#pragma once
#include "Prerequisites.h"

namespace Amadeus
{
	class Program
	{
	public:
		Program(const String& name);
		Program(String&& name);

		void Load(ShaderType type = ShaderType::TypeLess);

		void Unload();

		ID3DBlob* Get();

		uint64_t GetBufferSize();

		bool Loaded() { return bLoaded; }

	private:
		String mName;
		WString mPath;

		ShaderType mType = ShaderType::TypeLess;
		ComPtr<ID3DBlob> mShader;

		bool bLoaded = false;
	};
}