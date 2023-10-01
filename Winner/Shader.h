#pragma once
#include "Core.h"

class Shader : public NonCopyable
{
public:
	Shader() = delete;
	Shader(const std::wstring& Filename, const D3D_SHADER_MACRO* Defines, CRef<std::string> EntryPoint, CRef<std::string> Target);
	~Shader() { }

	FORCEINLINE ID3DBlob* GetByteCode() const { return ByteCode.Get(); }
	FORCEINLINE ID3DBlob* GetErrors() const { return Errors.Get(); }

private:
	WRLComPtr<ID3DBlob> ByteCode;
	WRLComPtr<ID3DBlob> Errors;
};

