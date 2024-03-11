#pragma once
#include "Core.h"
#include "Shader.h"
#include "UploadHelpers.h"
#include <unordered_map>

struct VoxelizeContstants
{
	float WorldBound;
};

struct MeshVoxelizerData 
{
	DirectX::XMFLOAT4X4 VoxelView = Useful::Identity4x4();
	DirectX::XMFLOAT4X4 VoxelProj = Useful::Identity4x4();
};

struct MipVoxelData 
{
	MipVoxelData(int _dim) : mipDim(_dim) {}
	int mipDim;
};

enum class VOLUME_TEXTURE_TYPE : UINT 
{ 
	ALBEDO = 0, 
	NORMAL, 
	EMISSIVE, 
	RADIANCE, 
	FLAG, 
	COUNT 
};

struct VoxelCoord
{
	UINT X, Y, Z;

	bool operator==(CRef<VoxelCoord> OtherCoord) const
	{
		return X == OtherCoord.X && Y == OtherCoord.Y && Z == OtherCoord.Z;
	}

	bool operator!=(CRef<VoxelCoord> OtherCoord) const
	{
		return !(*this == OtherCoord);
	}

	template<typename Lambda>
	bool All(Lambda&& Predicate) const
	{
		return Predicate(X) && Predicate(Y) && Predicate(Z);
	}
};

class VolumeTexture : public NonCopyable
{
	friend class MeshVoxelizer;
public:
	VolumeTexture(CRef<VoxelCoord> InSize);
	~VolumeTexture();

	ID3D12Resource* GetResource() const;
	FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetUAVHandleCPU() const { return UAVHandleCPU; }
	FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetUAVHandleGPU() const { return UAVHandleGPU; }
	FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandleCPU() const { return SRVHandleCPU; }
	FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandleGPU() const { return SRVHandleGPU; }
	FORCEINLINE UINT GetNumDescriptors() const { return NumDescriptors; }
	void SetupUAV(D3D12_CPU_DESCRIPTOR_HANDLE InUAVHandleCPU, D3D12_GPU_DESCRIPTOR_HANDLE InUAVHandleGPU);
	void SetupSRV(D3D12_CPU_DESCRIPTOR_HANDLE InSRVHandleCPU, D3D12_GPU_DESCRIPTOR_HANDLE InSRVHandleGPU);

	void Init();
	void OnResize(CRef<VoxelCoord> InNewSize);
	FORCEINLINE D3D12_VIEWPORT GetViewport() const { return Viewport; }
	FORCEINLINE D3D12_RECT GetScissorRect()const { return ScissorRect; }
private:
	void BuildUAVDescriptors();
	void BuildSRVDescriptors();
	void BuildResources();
	void UpdateRects();

	VoxelCoord Size;
	UINT NumDescriptors;
	D3D12_CPU_DESCRIPTOR_HANDLE UAVHandleCPU;
	D3D12_GPU_DESCRIPTOR_HANDLE UAVHandleGPU;
	D3D12_CPU_DESCRIPTOR_HANDLE SRVHandleCPU;
	D3D12_GPU_DESCRIPTOR_HANDLE SRVHandleGPU;
	DXGI_FORMAT TextureFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
	DXGI_FORMAT SRVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT UAVFormat = DXGI_FORMAT_R32_UINT;
	WRLComPtr<ID3D12Resource> Texture3DResource;
	D3D12_VIEWPORT Viewport;
	D3D12_RECT ScissorRect;
};

// TODO: duplicate code! not like this!
class RadianceMipMappedVolumeTexture : public NonCopyable
{
public:

	RadianceMipMappedVolumeTexture(CRef<VoxelCoord> InSize);
	~RadianceMipMappedVolumeTexture();

	ID3D12Resource* GetResource() const;
	//FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetUAVHandleCPU() const { return UAVHandleCPU; }
	//FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetUAVHandleGPU() const { return UAVHandleGPU; }
	FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandleCPU() const { return SRVHandleCPU; }
	FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandleGPU() const { return SRVHandleGPU; }
	FORCEINLINE UINT GetNumDescriptors() const { return NumDescriptors; }
	void SetupUAV(CD3DX12_CPU_DESCRIPTOR_HANDLE CPUHandleStart, CD3DX12_GPU_DESCRIPTOR_HANDLE GPUHandleStart);
	void SetupSRV(D3D12_CPU_DESCRIPTOR_HANDLE InSRVHandleCPU, D3D12_GPU_DESCRIPTOR_HANDLE InSRVHandleGPU);
	FORCEINLINE UINT GetNumMipLevels() const { return NumMipLevels; }

	void Init();
	void OnResize(CRef<VoxelCoord> InSize);

private:
	void BuildUAVDescriptors();
	void BuildSRVDescriptors();
	void BuildResources();

	VoxelCoord Size;
	UINT NumDescriptors;
	UINT NumMipLevels;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> UAVHandleCPU;
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> UAVHandleGPU;
	D3D12_CPU_DESCRIPTOR_HANDLE SRVHandleCPU;
	D3D12_GPU_DESCRIPTOR_HANDLE SRVHandleGPU;
	DXGI_FORMAT TextureFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
	DXGI_FORMAT SRVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT UAVFormat = DXGI_FORMAT_R32_UINT;
	WRLComPtr<ID3D12Resource> Texture3DResource;
};


class MeshVoxelizer : public NonCopyable
{
public:
	struct Specification
	{
		VoxelCoord TextureSize;
		float SceneExtent = 0.f;
		const std::vector<D3D12_INPUT_ELEMENT_DESC>* InputLayout = nullptr;
		ID3D12GraphicsCommandList2* CommandList = nullptr;
		ID3D12DescriptorHeap* ConstantBufferHeap = nullptr;
		INT ConstBufferOffset = -1;
		ID3D12DescriptorHeap* UAVSRVHeap = nullptr;
		INT UAVSRVOffset = -1;
	};

	static constexpr size_t GetConstantBufferCount() { return 1; }
	static constexpr size_t GeUAVSRVCount() { return 2 + 2; }

	MeshVoxelizer(CRef<Specification> InSpec);
	~MeshVoxelizer();

	void Init();
	void OnResize(CRef<VoxelCoord> InSize);
	D3D12_VIEWPORT GetViewport() const;
	D3D12_RECT GetScissorRect() const;

	FORCEINLINE MeshVoxelizerData GetUniformData() const { return Data; }
	//FORCEINLINE UINT GetNumDescriptors() const { return NumDescriptors; }
	FORCEINLINE VoxelCoord GetSize() const { return Spec.TextureSize; }

	/*VolumeTexture* GetVolumeTexture(VOLUME_TEXTURE_TYPE Type) const;
	RadianceMipMappedVolumeTexture* GetRadianceMipMapedVolumeTexture() const;
	std::unordered_map<VOLUME_TEXTURE_TYPE, std::unique_ptr<VolumeTexture>>& GetVoxelTexturesMap() const;*/

	/*void Clear3DTexture(ID3D12GraphicsCommandList* CommandList, ID3D12RootSignature* RootSignature, ID3D12PipelineState* PSO);

	void SetUpDescriptors4Voxels(ID3D12DescriptorHeap* Heap);
	void SetUpDescriptors4RadianceMipMaped(ID3D12DescriptorHeap* Heap);*/
	FORCEINLINE ID3D12PipelineState* GetVoxelizePSO() const { return VoxelizePSO.Get(); }
	FORCEINLINE ID3D12PipelineState* GetVoxelizeRestPSO() const { return VoxelizeResetPSO.Get(); }
	FORCEINLINE ID3D12RootSignature* GetVoxelizeRootSignature() const { return VoxelizeRootSignature.Get(); }
	FORCEINLINE ID3D12RootSignature* GetVoxelizeResetRootSignature() const { return VoxelizeResetRootSignature.Get(); }
	FORCEINLINE VolumeTexture* GetAlbedoTexture() const { return AlbedoTexture.get(); }
	FORCEINLINE VolumeTexture* GetNormalTexture() const { return NormalTexture.get(); }
	FORCEINLINE VoxelCoord GetTextureDimension() const { return AlbedoTexture.get()->Size; }
	FORCEINLINE CD3DX12_GPU_DESCRIPTOR_HANDLE GetConstHandleGPU() const { return ConstantHandleGPU; }

private:

	void PopulateUniformData();
	void CreateShaders();
	void BuildRootSignatures();
	void BuildPSOs();
	void BuildConstants();
	void BuildSRVs();

	//VoxelCoord Size;
	Specification Spec;
	//UINT NumDescriptors;
	//std::unordered_map<VOLUME_TEXTURE_TYPE, std::unique_ptr<VolumeTexture>> VolumeTextures;
	std::unique_ptr<RadianceMipMappedVolumeTexture> RadianceMipMapedTexture;

	D3D12_VIEWPORT Viewport;
	D3D12_RECT ScissorRect;
	MeshVoxelizerData Data;
	
	std::unique_ptr< BufferUploader< VoxelizeContstants, true> > VoxelConstantUploader;
	WRLComPtr<ID3D12RootSignature> VoxelizeRootSignature, VoxelizeResetRootSignature;
	std::unique_ptr<Shader> VoxelizeVertexShader, VoxelizeGeoShader, VoxelizePixelShader, VoxelizerResetCS;
	WRLComPtr<ID3D12PipelineState> VoxelizePSO, VoxelizeResetPSO;
	std::unique_ptr< VolumeTexture > AlbedoTexture, NormalTexture;
	CD3DX12_GPU_DESCRIPTOR_HANDLE ConstantHandleGPU;
};
