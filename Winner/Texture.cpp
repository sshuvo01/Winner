#include "Texture.h"
#include <DirectXTex/DirectXTex/DirectXTex.h>

Texture::Texture()
{
}

Texture::~Texture()
{
}

void Texture::Load(CRef<std::wstring> InFilename, ID3D12Device* Device, ID3D12GraphicsCommandList2* CmdList)
{
	Filename = InFilename;
	LoadAndUploadTexture(Device, CmdList);
}

void Texture::LoadAndUploadTexture(ID3D12Device* Device, ID3D12GraphicsCommandList2* CmdList)
{
	using namespace DirectX;
	TexMetadata Metadata;
	ScratchImage ScratchImage;

	if (Filename.find(L".dds") != std::string::npos)
	{
		// Use DDS texture loader.
		ThrowIfFailed(LoadFromDDSFile(Filename.c_str(), DDS_FLAGS_NONE, &Metadata, ScratchImage));
	}
	else if (Filename.find(L".hdr") != std::string::npos)
	{
		ThrowIfFailed(LoadFromHDRFile(Filename.c_str(), &Metadata, ScratchImage));
	}
	else if (Filename.find(L".tga") != std::string::npos)
	{
		ThrowIfFailed(LoadFromTGAFile(Filename.c_str(), &Metadata, ScratchImage));
	}
	else
	{
		ThrowIfFailed(LoadFromWICFile(Filename.c_str(), WIC_FLAGS_NONE, &Metadata, ScratchImage));
	}

	AvailableMipLevels = static_cast<UINT32>(Metadata.mipLevels);
	// TODO: ?
	CD3DX12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(Metadata.format, static_cast<UINT64>(Metadata.width), static_cast<UINT>(Metadata.height), static_cast<UINT16>(Metadata.arraySize));
	
	std::vector<D3D12_SUBRESOURCE_DATA> Subresources(ScratchImage.GetImageCount());
	const Image* Images = ScratchImage.GetImages();
	for (size_t i = 0; i < ScratchImage.GetImageCount(); ++i)
	{
		D3D12_SUBRESOURCE_DATA& Subresource = Subresources[i];
		Subresource.RowPitch = Images[i].rowPitch;
		Subresource.SlicePitch = Images[i].slicePitch;
		Subresource.pData = Images[i].pixels;
	}

	TextureDataUploader = std::make_unique<DefaultBufferUploader>();
	TextureDataUploader->CreateAndUpload(Device, CmdList, &textureDesc, 0, static_cast<UINT32>(Subresources.size()), Subresources.data());
}
