#pragma once
#include "UploadHelpers.h"
#include "Core.h"

class Texture : public NonCopyable
{
public:
	Texture();
	~Texture();

	void Load(CRef<std::wstring> InFilename, ID3D12Device* Device, ID3D12GraphicsCommandList2* CmdList);
	FORCEINLINE UINT32 GetAvailableMipLevels() const { return AvailableMipLevels; }
	FORCEINLINE ID3D12Resource* GetDefaultBuffer() const { return TextureDataUploader->GetDefaultBuffer(); }
private:
	std::unique_ptr<DefaultBufferUploader> TextureDataUploader;
	std::wstring Filename;
	UINT32 AvailableMipLevels = 0;

	void LoadAndUploadTexture(ID3D12Device* Device, ID3D12GraphicsCommandList2* CmdList);
};

