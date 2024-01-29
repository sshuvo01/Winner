//#pragma comment(lib, "windowscodecs.lib")
//#pragma comment(lib, "dxguid.lib") 
#define WIN32_LEAN_AND_MEAN
#define _ENABLE_EXTENDED_ALIGNED_STORAGE 
//to acknowledge that you understand this message and that you actually want a type with an extended alignment, or (2) _DISABLE_EXTENDED_ALIGNED_STORAGE to silence this message and get the old non-conformant behavior.	Winner	c:\program files (x86)\microsoft visual studio\2017\community\vc\tools\msvc\14.16.27023\include\type_traits	1271	

#include <Windows.h>
#include <Shlwapi.h>

#include "Application.h"

#include <dxgidebug.h>

#include "Core.h"
#include "UploadHelpers.h"
#include "GoodGame.h"

void ReportLiveObjects()
{
	IDXGIDebug1* dxgiDebug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

	dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	dxgiDebug->Release();
}

int CALLBACK main(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
	int retCode = 0;

	// Set the working directory to the path of the executable.
	WCHAR path[MAX_PATH];
	HMODULE hModule = GetModuleHandleW(NULL);
	if (GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
	{
		PathRemoveFileSpecW(path);
		SetCurrentDirectoryW(path);
	}

	Application::Create(hInstance);
	{
		//std::shared_ptr<Tutorial2> demo = std::make_shared<Tutorial2>(L"Good Game", 1280, 720, false);
		std::shared_ptr<GoodGame> demo = std::make_shared<GoodGame>(L"Good Game", 1280, 720, false);
		retCode = Application::Get().Run(demo);
	}
	Application::Destroy();

	atexit(&ReportLiveObjects);

	return retCode;
}