#pragma once
#include "Core.h"
#include "d3dx12.h"

class DefaultBufferUploader : public NonCopyable
{
public:
	DefaultBufferUploader() { }
	~DefaultBufferUploader() { }

	// TODO: organize these!
	void CreateAndUpload(ID3D12Device* Device, ID3D12GraphicsCommandList2* CmdList, const void* BufferData, UINT64 ByteSize);
	void CreateAndUpload(ID3D12Device* Device, ID3D12GraphicsCommandList2* CmdList, const D3D12_RESOURCE_DESC* ResourceDesc, 
		UINT32 FirstSubresource, UINT32 NumSubresources, D3D12_SUBRESOURCE_DATA* SubresourceData);
	
	FORCEINLINE ID3D12Resource* GetDefaultBuffer() const { assert(DefaultBuffer); return DefaultBuffer.Get(); }
	FORCEINLINE ID3D12Resource* GetUploadBuffer() const { assert(UploadBuffer); return UploadBuffer.Get(); }

private:
	WRLComPtr<ID3D12Resource> DefaultBuffer;
	WRLComPtr<ID3D12Resource> UploadBuffer;
};

template<typename T, bool _Constant = true>
class BufferUploader : public NonCopyable
{
public:
	BufferUploader(ID3D12Device* Device, UINT64 InElementCount)
		: ElementCount(InElementCount)
	{
		ElementSize = sizeof(T);
		
		if (IsConstant)
		{
			ElementSize = Useful::GetConstantBufferByteSize(ElementSize);
		}

		ThrowIfFailed(
		Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(ElementSize * ElementCount), 
			D3D12_RESOURCE_STATE_GENERIC_READ, 
			nullptr,
			IID_PPV_ARGS(UploadBuffer.GetAddressOf())) // Note: hmm
		);

		ThrowIfFailed(
			UploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&MappedData))
		);
	}

	void CopyData(const void* Buffer, UINT64 ElementIndex)
	{
		memcpy(&MappedData[ElementIndex * ElementSize], Buffer, ElementSize);
	}

	FORCEINLINE ID3D12Resource* GetBuffer() const { return UploadBuffer.Get(); }

	~BufferUploader()
	{
		if (UploadBuffer)
		{
			UploadBuffer->Unmap(0, nullptr);
		}

		UploadBuffer = nullptr;
	}

	FORCEINLINE ID3D12Resource* GetUploadBuffer() const { return UploadBuffer.Get(); }
	FORCEINLINE UINT GetElementSize() const { return ElementSize; }

private:
	BYTE* MappedData = nullptr;
	WRLComPtr<ID3D12Resource> UploadBuffer;
	UINT ElementSize = 0;
	const bool IsConstant = _Constant;
	const UINT64 ElementCount = _ElementCount;
};

