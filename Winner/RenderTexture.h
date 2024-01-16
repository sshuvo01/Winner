#pragma once
#include "Core.h"

class RenderTexture 
{
public:
	struct Specification
	{
		enum class Type : UINT
		{
			RenderTarget = 0,
			ShaderResource = 1,
			UnorderedAccess = 2,
			Max = 3
		} Type_ = Type::RenderTarget;

		UINT Width;
		UINT Height;
		DXGI_FORMAT Format;

	};

	RenderTexture();
	FORCEINLINE Microsoft::WRL::ComPtr<ID3D12Resource> GetResource() const { return Resource; }
	FORCEINLINE Specification GetSpec() const { return Spec; }
	FORCEINLINE CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const { return CPUHandle; }
	FORCEINLINE CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const { return GPUHandle; }
	static void ReleaseHeaps(); // TODO: not like this!

	void BuildDescriptor();
	void BuildResource(CRef<Specification> InSpec, Microsoft::WRL::ComPtr<ID3D12Resource> InResource);

	static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescHeaps[(UINT)Specification::Type::Max];

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
	CD3DX12_CPU_DESCRIPTOR_HANDLE CPUHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GPUHandle;
	Specification Spec;
	constexpr static UINT MaxDesc = 5; // TODO: find a better way to do this!
	static bool bInitialized; // TODO:
	static UINT RenderTargetIndex; // TODO: 
	static UINT ShaderResourceIndex; // TODO: 
	static UINT UnorderedAccessIndex; // TODO:


};

