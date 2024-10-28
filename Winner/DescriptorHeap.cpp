#include "DescriptorHeap.h"
#include <Application.h>

DescriptorHeap::DescriptorHeap()
	: IsReady{ false }, IncrementSize { 0 }
{
}

DescriptorHeap::DescriptorHeap(CRef<D3D12_DESCRIPTOR_HEAP_DESC> InDesc)
	: IsReady{ false }, IncrementSize{ 0 }
{
	Create(InDesc);
}

DescriptorHeap::~DescriptorHeap()
{
}

void DescriptorHeap::Create(CRef<D3D12_DESCRIPTOR_HEAP_DESC> InDesc)
{
	ASSERTBREAK(!IsReady);
	Desc = InDesc;
	ThrowIfFailed(Application::Get().GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(DescriptorHeapYo.GetAddressOf())));
	IncrementSize = Application::Get().GetDescriptorHandleIncrementSize(Desc.Type);
	IsReady = true;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUHandle(UINT Offset) const
{
	ASSERTBREAK(IsReady && Offset < Desc.NumDescriptors);
	CD3DX12_CPU_DESCRIPTOR_HANDLE Handle{ DescriptorHeapYo->GetCPUDescriptorHandleForHeapStart() };
	Handle.Offset(Offset, IncrementSize);
	return Handle;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUHandle(UINT Offset) const
{
	ASSERTBREAK(IsReady && Offset < Desc.NumDescriptors);
	CD3DX12_GPU_DESCRIPTOR_HANDLE Handle{ DescriptorHeapYo->GetGPUDescriptorHandleForHeapStart() };
	Handle.Offset(Offset, IncrementSize);
	return Handle; 
}

DescriptorHandleDesc DescriptorHeapManager::RegisterDescriptor(CRef<HeapRegisterDesc> InDesc)
{
	// TODO: put some validation here!
	DescriptorHandleDesc Handle;
	Handle.Desc = InDesc;
	Handle.Offset = DescriptorCountMap[InDesc]++;
	//DescriptorCountMap[InDesc] = 89;
	//UINT asfa = DescriptorCountMap[InDesc];
	return Handle;
}

DescriptorHeapManager::DescriptorHeapManager()
{
	/*for (INT Iter = 0; Iter < (INT) D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; Iter++)
	{
		DescriptorCountMap[(D3D12_DESCRIPTOR_HEAP_TYPE) Iter] = 0;
	}*/
}

DescriptorHeapManager::~DescriptorHeapManager()
{
}

DescriptorHeapManager* DescriptorHeapManager::Get()
{
	static DescriptorHeapManager Singleton;
	return &Singleton;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetCPUHandle(CRef<DescriptorHandleDesc> InHandleDesc) const
{
	return GetDescriptorHeap(InHandleDesc)->GetCPUHandle(InHandleDesc.Offset);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetGPUHandle(CRef<DescriptorHandleDesc> InHandleDesc) const
{
	//DescriptorHeapsMap.find(InHandleDesc.Desc) != DescriptorHeapsMap.cend();
	/*const auto& Iter = DescriptorHeapsMap.find(InHandleDesc.Desc);
	ASSERTBREAK(Iter != DescriptorHeapsMap.end());
	return Iter->second->GetGPUHandle(InHandleDesc.Offset);*/
	return GetDescriptorHeap(InHandleDesc)->GetGPUHandle(InHandleDesc.Offset);
}

void DescriptorHeapManager::AddHeapResourceView(HeapResourceView::HeapResourceViewPtr InResourceView)
{
	ResourceViews.push_back(std::move(InResourceView));
}

void DescriptorHeapManager::OnLoadingFinished()
{
	for (auto Iter = DescriptorCountMap.cbegin(); Iter != DescriptorCountMap.cend(); Iter++)
	{	
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc;
		HeapDesc.Type = Iter->first.Type;
		HeapDesc.NumDescriptors = Iter->second;
		HeapDesc.Flags = Iter->first.Flags;
		HeapDesc.NodeMask = Iter->first.NodeMask;

		DescriptorHeapsMap[Iter->first] = std::make_unique<DescriptorHeap>(HeapDesc);
		//DescriptorHeapsMap[Iter->first] = new DescriptorHeap(HeapDesc); // TODO: Free this in the descrutor
	}

	for (auto Iter = ResourceViews.cbegin(); Iter != ResourceViews.cend(); Iter++)
	{
		(*Iter)->CreateView();
	}
}

DescriptorHeap* DescriptorHeapManager::GetDescriptorHeap(CRef<DescriptorHandleDesc> InHandleDesc) const
{
	auto Iter = DescriptorHeapsMap.find(InHandleDesc.Desc);
	ASSERTBREAK(Iter != DescriptorHeapsMap.end());
	return Iter->second.get();
}

//DescriptorHeapManager::HeapType DescriptorHeapManager::GetHeapType(CRef<D3D12_DESCRIPTOR_HEAP_DESC> InDesc)
//{
//	switch (InDesc.Type)
//	{
//	case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
//		DescriptorHeapManager::HeapType::
//		break;
//	case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
//
//		break;
//	default:
//		break;
//	}
//}

HeapResourceView::HeapResourceView(CRef<DescriptorHandleDesc> InHandleDesc)
	: HandleDesc{ InHandleDesc }
{
}

HeapResourceView::~HeapResourceView()
{
}

HeapShaderResourceView::HeapShaderResourceView(CRef<DescriptorHandleDesc> InHandleDesc, ID3D12Resource* InResource, CRef<D3D12_SHADER_RESOURCE_VIEW_DESC> InShaderResourceDesc)
	: HeapResourceView(InHandleDesc),
	Resource{InResource},
	ShaderResourceDesc{InShaderResourceDesc}

{
}

HeapShaderResourceView::~HeapShaderResourceView()
{
}

void HeapShaderResourceView::CreateView()
{
}

HeapConstResourceView::HeapConstResourceView(CRef<DescriptorHandleDesc> InHandleDesc, CRef<D3D12_CONSTANT_BUFFER_VIEW_DESC> InConstantBufferView)
	: HeapResourceView(InHandleDesc),
	ConstantBufferView{ InConstantBufferView }
{
}

HeapConstResourceView::~HeapConstResourceView()
{
}

void HeapConstResourceView::CreateView()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE Handle = DescriptorHeapManager::Get()->GetCPUHandle(HandleDesc);
	Application::Get().GetDevice()->CreateConstantBufferView(&ConstantBufferView, Handle);
}
