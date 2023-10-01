#include "Shader.h"
#include <d3dcompiler.h>
#include <fstream>
#include <direct.h>
//MYMACRO = R"($(ProjectDir))"
Shader::Shader(const std::wstring & Filename, const D3D_SHADER_MACRO * Defines, CRef<std::string> EntryPoint, CRef<std::string> Target)
{
	UINT CompileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	CompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	
	HRESULT Hr = D3DCompileFromFile(Filename.c_str(), Defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		EntryPoint.c_str(), Target.c_str(), CompileFlags, 0, &ByteCode, &Errors);

	if (Errors != nullptr)
	{
		OutputDebugStringA(static_cast<CHAR*>(Errors->GetBufferPointer()));
		ASSERTNOENTRY("No!");
	}

	ThrowIfFailed(Hr);
}
