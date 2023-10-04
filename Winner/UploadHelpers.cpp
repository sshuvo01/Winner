#include "UploadHelpers.h"

void DefaultBufferUploader::CreateAndUpload(ID3D12Device* Device, ID3D12GraphicsCommandList2* CmdList, const void* BufferData, UINT64 ByteSize)
{
	DefaultBuffer = nullptr;
	UploadBuffer = nullptr;

	ThrowIfFailed(Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
			D3D12_HEAP_FLAG_NONE, 
			&CD3DX12_RESOURCE_DESC::Buffer(ByteSize), 
			D3D12_RESOURCE_STATE_COMMON, 
			nullptr, 
			IID_PPV_ARGS(DefaultBuffer.GetAddressOf())));

	ThrowIfFailed(Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
		D3D12_HEAP_FLAG_NONE, 
		&CD3DX12_RESOURCE_DESC::Buffer(ByteSize), 
		D3D12_RESOURCE_STATE_GENERIC_READ, 
		nullptr, 
		IID_PPV_ARGS(UploadBuffer.GetAddressOf())));

	D3D12_SUBRESOURCE_DATA SubResourceData;
	SubResourceData.pData = BufferData;
	SubResourceData.RowPitch = ByteSize;
	SubResourceData.SlicePitch = ByteSize;
	
	// Barrier for the default buffer common -> copy destination
	CD3DX12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(DefaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	CmdList->ResourceBarrier(1, &Barrier);
	UpdateSubresources<1>(CmdList, DefaultBuffer.Get(), UploadBuffer.Get(), 0, 0, 1, &SubResourceData);
	// Barrier for the default buffer copy destination -> read
	Barrier = CD3DX12_RESOURCE_BARRIER::Transition(DefaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	CmdList->ResourceBarrier(1, &Barrier);
}

void DefaultBufferUploader::CreateAndUpload(ID3D12Device * Device, ID3D12GraphicsCommandList2 * CmdList, 
	const D3D12_RESOURCE_DESC* ResourceDesc, UINT32 FirstSubresource, UINT32 NumSubresources, D3D12_SUBRESOURCE_DATA * SubresourceData)
{
	DefaultBuffer = nullptr;
	UploadBuffer = nullptr;

	ThrowIfFailed(Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		ResourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(DefaultBuffer.GetAddressOf())));

	const UINT64 RequiredSize = GetRequiredIntermediateSize(DefaultBuffer.Get(), FirstSubresource, NumSubresources);
	ThrowIfFailed(Device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(RequiredSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(UploadBuffer.GetAddressOf())
	));
	
	// Barrier for the default buffer common -> copy destination
	CD3DX12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(DefaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	CmdList->ResourceBarrier(1, &Barrier);
	
	UpdateSubresources(CmdList, DefaultBuffer.Get(), UploadBuffer.Get(), 0, FirstSubresource, NumSubresources, SubresourceData);

	Barrier = CD3DX12_RESOURCE_BARRIER::Transition(DefaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	CmdList->ResourceBarrier(1, &Barrier);
}