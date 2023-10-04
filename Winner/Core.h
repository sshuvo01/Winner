#pragma once
#include <iostream>
#include <string>
#include <d3dcommon.h>
#include <wrl/client.h>
#include <cassert>
#include <DirectXMath.h>
#include <vector>

#include "d3dx12.h"
#include "Helpers.h"

/*
char* buffer;

	// Get the current working directory:
	if ((buffer = _getcwd(NULL, 0)) == NULL)
		perror("_getcwd error");
	else
	{
		printf("%s \nLength: %zu\n", buffer, strlen(buffer));
		free(buffer);
	}
*/

template<typename T>
struct ResourceDirectory
{
	static T GetPath()
	{
		static_assert(false, "No!");
		return {};
	}
};

template<>
struct ResourceDirectory<std::string>
{
	static std::string GetPath()
	{
		return RESOURCEDIR;
	}
};

template<>
struct ResourceDirectory<std::wstring>
{
	static std::wstring GetPath()
	{
		static std::wstring Path;
		if (!Path.length())
		{
			std::string PathStr = RESOURCEDIR;
			for (char Ch : PathStr)
			{
				Path += static_cast<WCHAR>(Ch);
			}
		}

		return Path;
	}
};

template<typename T>
using WRLComPtr = Microsoft::WRL::ComPtr<T>;

template<typename T>
using CRef = const T&;

#define ASSERTNOENTRY(Msg) assert(false && Msg)

class NonCopyable
{
public:
	NonCopyable() = default;
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;
	virtual ~NonCopyable() { }
};

class Test : public NonCopyable
{
public:
	int Yo;
	Test(int InYo) : Yo{InYo}
	{
	}

	~Test() 
	{
		std::cout << "Destroying Test" << std::endl;
	}
};

class Useful
{
public:
	static UINT GetConstantBufferByteSize(UINT ByteSize);

	template<UINT ByteSize>
	static constexpr UINT GetConstantBufferByteSize()
	{
		return (ByteSize + 255) & ~255;
	}

	static DirectX::XMFLOAT4X4 Identity4x4() noexcept
	{
		static DirectX::XMFLOAT4X4 I(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);

		return I;
	}

	constexpr static float PI = 3.1415926535f;

};