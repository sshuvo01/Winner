#pragma once
#include "Core.h"
#include <unordered_map>

struct HeapRegisterDesc : public D3D12_DESCRIPTOR_HEAP_DESC
{
	bool operator==(CRef<HeapRegisterDesc> Rhs) const
	{
		return Type == Rhs.Type && Flags == Rhs.Flags && NodeMask == Rhs.NodeMask;
	}

	bool operator!=(CRef<HeapRegisterDesc> Rhs) const
	{
		return !(*this == Rhs);
	}
};

struct DescriptorHandleDesc
{
	HeapRegisterDesc Desc;
	UINT Offset = 0;
};

class HeapResourceView : public NonCopyable
{
public:
	HeapResourceView(CRef<DescriptorHandleDesc> InHandleDesc);
	virtual ~HeapResourceView();
	virtual void CreateView() = 0;
	using HeapResourceViewPtr = std::unique_ptr<HeapResourceView>;
protected: 
	DescriptorHandleDesc HandleDesc;
	friend class DescriptorHeapManager;
};

class HeapConstResourceView : public HeapResourceView
{
public:
	HeapConstResourceView(CRef<DescriptorHandleDesc> InHandleDesc, CRef <D3D12_CONSTANT_BUFFER_VIEW_DESC> InConstantBufferView);
	~HeapConstResourceView();
	virtual void CreateView() override;
private:
	D3D12_CONSTANT_BUFFER_VIEW_DESC ConstantBufferView;
};

class HeapShaderResourceView : public HeapResourceView
{
public:
	HeapShaderResourceView(CRef<DescriptorHandleDesc> InHandleDesc, ID3D12Resource* InResource, CRef<D3D12_SHADER_RESOURCE_VIEW_DESC> InShaderResourceDesc);
	~HeapShaderResourceView();
	virtual void CreateView() override;
private:
	ID3D12Resource* Resource;
	D3D12_SHADER_RESOURCE_VIEW_DESC ShaderResourceDesc;
};


class DescriptorHeap : public NonCopyable
{
public:

	DescriptorHeap();
	DescriptorHeap(CRef<D3D12_DESCRIPTOR_HEAP_DESC> InDesc);
	~DescriptorHeap();
	void Create(CRef<D3D12_DESCRIPTOR_HEAP_DESC> InDesc);
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT Offset) const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(UINT Offset) const; 
	FORCEINLINE UINT GetDescriptorCount() const { ASSERTBREAK(IsReady); return Desc.NumDescriptors; }
	FORCEINLINE D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { ASSERTBREAK(IsReady); return Desc.Type; }
	FORCEINLINE ID3D12DescriptorHeap* GetHeap() const { return DescriptorHeapYo.Get(); }
	//FORCEINLINE ID3D12DescriptorHeap** GetHeapAddressOf()  { return DescriptorHeapYo.GetAddressOf(); }
private: 
	D3D12_DESCRIPTOR_HEAP_DESC Desc;
	WRLComPtr<ID3D12DescriptorHeap> DescriptorHeapYo;
	bool IsReady = false;
	UINT IncrementSize;

	friend class DescriptorHeapManager;
};

using DescriptorHeapUPtr = std::unique_ptr< DescriptorHeap >;
struct HeapRegisterDescHash
{
	std::size_t operator()(const HeapRegisterDesc& RegisterDesc) const noexcept
	{
		std::size_t H1 = std::hash<D3D12_DESCRIPTOR_HEAP_TYPE>{}(RegisterDesc.Type);
		std::size_t H2 = std::hash<D3D12_DESCRIPTOR_HEAP_FLAGS>{}(RegisterDesc.Flags);
		std::size_t H3 = std::hash<UINT>{}(RegisterDesc.NodeMask);
		return H1 ^ ((H2 << 1) >> 1) ^ (H3 << 1);
	}
};
//D3D12_DESCRIPTOR_HEAP_TYPE_RTV
//D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
/*enum class HeapType : UINT8
{
	D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV = 0,
	D3D12_DESCRIPTOR_HEAP_TYPE_RTV = 1,
	Num = 2
};*/

class DescriptorHeapManager : public NonCopyable
{
public:

	DescriptorHandleDesc RegisterDescriptor(CRef<HeapRegisterDesc> InDesc);
	~DescriptorHeapManager();

	static DescriptorHeapManager* Get();
	//static HeapType GetHeapType(CRef<D3D12_DESCRIPTOR_HEAP_DESC> InDesc);
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(CRef<DescriptorHandleDesc> InHandleDesc) const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(CRef<DescriptorHandleDesc> InHandleDesc) const;
	void AddHeapResourceView(HeapResourceView::HeapResourceViewPtr InResourceView);
	void OnLoadingFinished();
private:
	mutable std::unordered_map<HeapRegisterDesc, UINT, HeapRegisterDescHash> DescriptorCountMap;
	mutable std::unordered_map<HeapRegisterDesc, DescriptorHeapUPtr, HeapRegisterDescHash> DescriptorHeapsMap;
	std::vector<HeapResourceView::HeapResourceViewPtr> ResourceViews;
	DescriptorHeapManager();

	DescriptorHeap* GetDescriptorHeap(CRef<DescriptorHandleDesc> InHandleDesc) const;
};