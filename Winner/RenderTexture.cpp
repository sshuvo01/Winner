#include "RenderTexture.h"
#include "Application.h"

bool RenderTexture::bInitialized = false;
UINT RenderTexture::RenderTargetIndex = 0;
UINT RenderTexture::ShaderResourceIndex = 0;
UINT RenderTexture::UnorderedAccessIndex = 0;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RenderTexture::DescHeaps[(UINT)RenderTexture::Specification::Type::Max];

RenderTexture::RenderTexture()
{
}

void RenderTexture::ReleaseHeaps()
{
	for (UINT i = 0; i < (UINT)Specification::Type::Max; i++)
	{
		DescHeaps[i].Reset();
	}
}

void RenderTexture::BuildDescriptor()
{
	Application& App = Application::Get();
	switch (Spec.Type_)
	{
	case Specification::Type::RenderTarget:
	{
		const UINT DescSize = App.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		CPUHandle = DescHeaps[(UINT)Specification::Type::RenderTarget]->GetCPUDescriptorHandleForHeapStart();
		CPUHandle.Offset(RenderTargetIndex, DescSize);
		GPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
			DescHeaps[(UINT)Specification::Type::RenderTarget]->GetGPUDescriptorHandleForHeapStart(),
			RenderTargetIndex,
			DescSize);
		RenderTargetIndex++;
		App.GetDevice()->CreateRenderTargetView(Resource.Get(), nullptr, CPUHandle);
		break;
	}
	case Specification::Type::ShaderResource:
	{
		const UINT DescSize = App.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
		SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SrvDesc.Format = Spec.Format;
		SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SrvDesc.Texture2D.MostDetailedMip = 0;
		SrvDesc.Texture2D.MipLevels = 1;

		CPUHandle = DescHeaps[(UINT)Specification::Type::ShaderResource]->GetCPUDescriptorHandleForHeapStart();
		CPUHandle.Offset(ShaderResourceIndex, DescSize);
		GPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
			DescHeaps[(UINT)Specification::Type::ShaderResource]->GetGPUDescriptorHandleForHeapStart(),
			ShaderResourceIndex, DescSize);
		ShaderResourceIndex++;
		App.GetDevice()->CreateShaderResourceView(Resource.Get(), &SrvDesc, CPUHandle);
		break;
	}
	case Specification::Type::UnorderedAccess:
	{
		const UINT DescSize = App.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = {};
		UavDesc.Format = Spec.Format;
		UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		UavDesc.Texture2D.MipSlice = 0;

		CPUHandle = DescHeaps[(UINT)Specification::Type::UnorderedAccess]->GetCPUDescriptorHandleForHeapStart();
		CPUHandle.Offset(UnorderedAccessIndex, DescSize);
		GPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
			DescHeaps[(UINT)Specification::Type::UnorderedAccess]->GetGPUDescriptorHandleForHeapStart(),
			UnorderedAccessIndex,
			DescSize
		);

		UnorderedAccessIndex++;
		App.GetDevice()->CreateUnorderedAccessView(Resource.Get(), nullptr, nullptr, CPUHandle);
		break;
	}
	default:
		ASSERTNOENTRY("NO!");
		break;
	}
}

void RenderTexture::BuildResource(CRef<Specification> InSpec, Microsoft::WRL::ComPtr<ID3D12Resource> InResource)
{
	Spec = InSpec;
	Application& App = Application::Get();

	if (!bInitialized)
	{
		DescHeaps[(UINT)Specification::Type::RenderTarget] = App.CreateDescriptorHeap(MaxDesc, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		DescHeaps[(UINT)Specification::Type::ShaderResource] = App.CreateDescriptorHeap(MaxDesc, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		DescHeaps[(UINT)Specification::Type::UnorderedAccess] = App.CreateDescriptorHeap(MaxDesc, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		// TODO: handle the other type(s) later
		bInitialized = true;
	}

	// Has to be separated... Resource!
	switch (Spec.Type_)
	{
	case Specification::Type::RenderTarget:
		ASSERTBREAK(InResource);
		if (InResource)
		{
			Resource = InResource;
		}
		break;
	case Specification::Type::ShaderResource:
	case Specification::Type::UnorderedAccess:
	{
		ASSERTBREAK(!InResource);
		D3D12_RESOURCE_DESC TexDesc;
		ZeroMemory(&TexDesc, sizeof(D3D12_RESOURCE_DESC));
		TexDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		TexDesc.Alignment = 0;
		TexDesc.Width = Spec.Width;
		TexDesc.Height = Spec.Height;
		TexDesc.DepthOrArraySize = 1;
		TexDesc.MipLevels = 1;
		TexDesc.Format = Spec.Format;
		TexDesc.SampleDesc.Count = 1;
		TexDesc.SampleDesc.Quality = 0;
		TexDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		TexDesc.Flags = Spec.Type_ == Specification::Type::UnorderedAccess ?
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

		ThrowIfFailed(App.GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&TexDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&Resource)));
		break;
	}
	default:
		ASSERTNOENTRY("NO!");
		break;
	}
}
