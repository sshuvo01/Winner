#include "Voxelizer.h"
#include "Core.h"
#include <Application.h>

VolumeTexture::VolumeTexture(CRef<VoxelCoord> InSize)
	: Size{ InSize }, NumDescriptors{ 2 }
{
	UpdateRects();
	Init();
}

VolumeTexture::~VolumeTexture()
{
}

ID3D12Resource* VolumeTexture::GetResource() const
{
	return Texture3DResource.Get();
}

void VolumeTexture::SetupUAV(D3D12_CPU_DESCRIPTOR_HANDLE InUAVHandleCPU, D3D12_GPU_DESCRIPTOR_HANDLE InUAVHandleGPU)
{
	UAVHandleCPU = InUAVHandleCPU;
	UAVHandleGPU = InUAVHandleGPU;

	BuildUAVDescriptors();
}

void VolumeTexture::SetupSRV(D3D12_CPU_DESCRIPTOR_HANDLE InSRVHandleCPU, D3D12_GPU_DESCRIPTOR_HANDLE InSRVHandleGPU)
{
	SRVHandleCPU = InSRVHandleCPU;
	SRVHandleGPU = InSRVHandleGPU;

	BuildSRVDescriptors();
}

void VolumeTexture::Init()
{
	BuildResources();
}

void VolumeTexture::OnResize(CRef<VoxelCoord> InNewSize)
{
	if (Size != InNewSize)
	{
		Size = InNewSize;
		UpdateRects();
		BuildResources();
		BuildUAVDescriptors();
		// BuildSRVDescriptors();// TODO: why not?
	}
}

void VolumeTexture::BuildUAVDescriptors()
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.Format = UAVFormat;
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
	UAVDesc.Texture3D.MipSlice = 0;
	UAVDesc.Texture3D.FirstWSlice = 0;
	UAVDesc.Texture3D.WSize = -1;
	
	Application::Get().GetDevice()->CreateUnorderedAccessView(Texture3DResource.Get(), nullptr, &UAVDesc, UAVHandleCPU);
}

void VolumeTexture::BuildSRVDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Format = SRVFormat;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	SRVDesc.Texture3D.MostDetailedMip = 0;
	SRVDesc.Texture3D.MipLevels = 1;
	SRVDesc.Texture3D.ResourceMinLODClamp = 0.0f;

	Application::Get().GetDevice()->CreateShaderResourceView(Texture3DResource.Get(), &SRVDesc, SRVHandleCPU);
}

void VolumeTexture::BuildResources()
{
	D3D12_RESOURCE_DESC TexDesc;
	ZeroMemory(&TexDesc, sizeof(D3D12_RESOURCE_DESC));
	TexDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	TexDesc.Alignment = 0;
	TexDesc.Width = Size.X;
	TexDesc.Height = Size.Y;
	TexDesc.DepthOrArraySize = Size.Z;
	TexDesc.MipLevels = 1;
	TexDesc.Format = TextureFormat;
	TexDesc.SampleDesc.Count = 1;
	TexDesc.SampleDesc.Quality = 0;
	TexDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	TexDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ThrowIfFailed(Application::Get().GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&TexDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(Texture3DResource.GetAddressOf())
	));
}

void VolumeTexture::UpdateRects()
{
	Viewport = { 0.f, 0.f, (FLOAT)Size.X, (FLOAT)Size.Y };
	ScissorRect = { 0, 0, (LONG)Size.X, (LONG)Size.Y };
}

RadianceMipMappedVolumeTexture::RadianceMipMappedVolumeTexture(CRef<VoxelCoord> InSize)
	: Size(InSize)
{
	// TODO: refactor this and find a better way!
	NumMipLevels = 0;
	VoxelCoord TempSize = Size;
	while (TempSize.X >= 1 && TempSize.Y >= 1 && TempSize.Z >= 1) 
	{
		NumMipLevels++;
		TempSize.X /= 2;
		TempSize.Y /= 2;
		TempSize.Z /= 2;
	}

	NumDescriptors = 2 * NumMipLevels;
}

RadianceMipMappedVolumeTexture::~RadianceMipMappedVolumeTexture()
{
}

ID3D12Resource* RadianceMipMappedVolumeTexture::GetResource() const
{
	return Texture3DResource.Get();
}

void RadianceMipMappedVolumeTexture::SetupUAV(CD3DX12_CPU_DESCRIPTOR_HANDLE CPUHandleStart, CD3DX12_GPU_DESCRIPTOR_HANDLE GPUHandleStart)
{
	const UINT UAVDescSize =  Application::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (UINT Iter = 0; Iter < NumMipLevels; Iter++)
	{
		CPUHandleStart.Offset(Iter, UAVDescSize);
		GPUHandleStart.Offset(Iter, UAVDescSize);

		UAVHandleCPU.push_back(CPUHandleStart);
		UAVHandleGPU.push_back(GPUHandleStart);
		/*auto curCPUHandle = heapPtr->mCPUHandle(heapPtr->getCurrentOffsetRef());
		auto curGPUHandle = heapPtr->mGPUHandle(heapPtr->getCurrentOffsetRef());
		mhCPUuavs.push_back(curCPUHandle);
		mhGPUuavs.push_back(curGPUHandle);
		heapPtr->incrementCurrentOffset();*/
	}

	BuildUAVDescriptors();
}

void RadianceMipMappedVolumeTexture::SetupSRV(D3D12_CPU_DESCRIPTOR_HANDLE InSRVHandleCPU, D3D12_GPU_DESCRIPTOR_HANDLE InSRVHandleGPU)
{
	SRVHandleCPU = InSRVHandleCPU;
	SRVHandleGPU = InSRVHandleGPU;

	BuildSRVDescriptors();
}

void RadianceMipMappedVolumeTexture::Init()
{
	BuildResources();
}

void RadianceMipMappedVolumeTexture::OnResize(CRef<VoxelCoord> InSize)
{
	if (Size != InSize) 
	{
		Size = InSize;

		BuildResources();
		BuildUAVDescriptors();
		// TODO: why not
	}
}

void RadianceMipMappedVolumeTexture::BuildUAVDescriptors()
{
	for (UINT Iter = 0; Iter < NumMipLevels; Iter++) 
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.Format = UAVFormat;
		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		UAVDesc.Texture3D.MipSlice = Iter;
		UAVDesc.Texture3D.FirstWSlice = 0;
		UAVDesc.Texture3D.WSize = -1;
		Application::Get().GetDevice()->CreateUnorderedAccessView(Texture3DResource.Get(), nullptr, &UAVDesc, UAVHandleCPU[Iter]);
	}
}

void RadianceMipMappedVolumeTexture::BuildSRVDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Format = SRVFormat;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	SRVDesc.Texture3D.MostDetailedMip = 0;
	SRVDesc.Texture3D.MipLevels = NumMipLevels;
	SRVDesc.Texture3D.ResourceMinLODClamp = 0.0f;
	Application::Get().GetDevice()->CreateShaderResourceView(Texture3DResource.Get(), &SRVDesc, SRVHandleCPU);
}

void RadianceMipMappedVolumeTexture::BuildResources()
{
	D3D12_RESOURCE_DESC TexDesc;
	ZeroMemory(&TexDesc, sizeof(D3D12_RESOURCE_DESC));
	TexDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	TexDesc.Alignment = 0;
	TexDesc.Width = (Size.X) * 6; // each anistropic mipmaping voxel needs 6 directions in total... wtf is this?
	TexDesc.Height = Size.Y;
	TexDesc.DepthOrArraySize = Size.Z;
	TexDesc.MipLevels = NumMipLevels;
	TexDesc.Format = TextureFormat;
	TexDesc.SampleDesc.Count = 1;
	TexDesc.SampleDesc.Quality = 0;
	TexDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	TexDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ThrowIfFailed(Application::Get().GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&TexDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(Texture3DResource.GetAddressOf())
	));
}

MeshVoxelizer::MeshVoxelizer(CRef<Specification> InSpec)
	: Spec{ InSpec }
{
	ASSERTBREAK(Spec.InputLayout && Spec.SceneExtent > 0.f);
	ASSERTBREAK(Spec.TextureSize.All([](UINT Component) { return Component > 0; }));
	ASSERTBREAK(Spec.CommandList && Spec.ConstantBufferHeap && Spec.ConstBufferOffset >= 0 && Spec.UAVSRVHeap && Spec.UAVSRVOffset >= 0);
	
	Viewport = { 0.f, 0.f, (FLOAT)Spec.TextureSize.X, (FLOAT)Spec.TextureSize.Y };
	ScissorRect = { 0, 0, (INT)Spec.TextureSize.X, (INT)Spec.TextureSize.Y };
	AlbedoTexture = std::make_unique<VolumeTexture>(Spec.TextureSize);
	NormalTexture = std::make_unique<VolumeTexture>(Spec.TextureSize);

	PopulateUniformData();
	CreateShaders();
	BuildRootSignatures();
	BuildPSOs();
	BuildSRVs();
	BuildConstants();
}

MeshVoxelizer::~MeshVoxelizer()
{
}

void MeshVoxelizer::Init()
{
	/*using namespace std;
	auto V_Albedo = make_unique<VolumeTexture>(Spec.TextureSize);
	auto V_Normal = make_unique<VolumeTexture>(Spec.TextureSize);
	auto V_Emissive = make_unique<VolumeTexture>(Spec.TextureSize);
	auto V_Radiance = make_unique<VolumeTexture>(Spec.TextureSize);
	auto V_Flag = make_unique<VolumeTexture>(Spec.TextureSize);
	VolumeTextures[VOLUME_TEXTURE_TYPE::ALBEDO] = move(V_Albedo);
	VolumeTextures[VOLUME_TEXTURE_TYPE::NORMAL] = move(V_Normal);
	VolumeTextures[VOLUME_TEXTURE_TYPE::EMISSIVE] = move(V_Emissive);
	VolumeTextures[VOLUME_TEXTURE_TYPE::RADIANCE] = move(V_Radiance);
	VolumeTextures[VOLUME_TEXTURE_TYPE::FLAG] = move(V_Flag);

	for (auto& VTexture : VolumeTextures)
	{
		VTexture.second->Init();
		NumDescriptors += VTexture.second->GetNumDescriptors();
	}

	RadianceMipMapedTexture = make_unique<RadianceMipMappedVolumeTexture>(Spec.TextureSize);
	RadianceMipMapedTexture->Init();
	NumDescriptors += RadianceMipMapedTexture->GetNumDescriptors();*/
}

void MeshVoxelizer::OnResize(CRef<VoxelCoord> InSize)
{
}

D3D12_VIEWPORT MeshVoxelizer::GetViewport() const
{
	return 	Viewport;
}

D3D12_RECT MeshVoxelizer::GetScissorRect() const
{
	return ScissorRect;
}

//MeshVoxelizerData MeshVoxelizer::GetUniformData() const
//{
//	return MeshVoxelizerData();
//}

void MeshVoxelizer::PopulateUniformData()
{
	using namespace DirectX;

	// SceneRadius = 400.0f; // TODO: find out what this is
	//XMFLOAT3 EyePos = XMFLOAT3(0.0f, 3.0, -Spec.SceneExtent); // TODO: find out what this is
	//XMFLOAT3 TartPos = XMFLOAT3(0.0f, 0.0f, 0.0f); // TODO: find out what this is
	//XMFLOAT3 Up = XMFLOAT3(0.0f, 1.0f, 0.0f); // TODO: find out what this is

	XMVECTOR Pos = DirectX::XMVectorSet(5.f, 6.f, 3.f, 1.f);
	XMVECTOR Target = DirectX::XMVectorZero();
	XMVECTOR Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX VoxelView = XMMatrixLookAtLH(Pos, Target, Up);

	//XMMATRIX VoxelView = XMMatrixLookAtLH(XMLoadFloat3(&EyePos), XMLoadFloat3(&TartPos), XMLoadFloat3(&Up));
	XMMATRIX VoxelProj = XMMatrixOrthographicLH(Spec.SceneExtent, Spec.SceneExtent, 0.0f, 1000.0f);

	XMStoreFloat4x4(&Data.VoxelView, VoxelView);
	XMStoreFloat4x4(&Data.VoxelProj, VoxelProj);
}

void MeshVoxelizer::BuildRootSignatures()
{
	WRLComPtr<ID3D12Device2> Device = Application::Get().GetDevice();
	{
		const int NumOfParams = 3 + 1 + 1; // 3 constant buffers, 1 texture, 2 rw (2 in 1)
		CD3DX12_ROOT_PARAMETER RootParameter[NumOfParams]; 
	
		// TODO: how to combine these 3 constant things?
		{
			CD3DX12_DESCRIPTOR_RANGE CbvTablePerObj;
			CbvTablePerObj.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // b0
			RootParameter[0].InitAsDescriptorTable(1, &CbvTablePerObj);
		}

		{
			CD3DX12_DESCRIPTOR_RANGE CbvTablePerFrame;
			CbvTablePerFrame.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // b1
			RootParameter[1].InitAsDescriptorTable(1, &CbvTablePerFrame);
		}

		{
			CD3DX12_DESCRIPTOR_RANGE CbvTableVoxelizer;
			CbvTableVoxelizer.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2); // b2
			RootParameter[2].InitAsDescriptorTable(1, &CbvTableVoxelizer);
		}

		{
			CD3DX12_DESCRIPTOR_RANGE TexDiffuse;
			TexDiffuse.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0
			RootParameter[3].InitAsDescriptorTable(1, &TexDiffuse, D3D12_SHADER_VISIBILITY_PIXEL);
		}

		{
			CD3DX12_DESCRIPTOR_RANGE TexVoxelizer3d;
			TexVoxelizer3d.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0); // u0, u1
			RootParameter[4].InitAsDescriptorTable(1, &TexVoxelizer3d, D3D12_SHADER_VISIBILITY_PIXEL);
		}

		auto StaticSamplers = Useful::GetCommonStaticSamplers();
		CD3DX12_ROOT_SIGNATURE_DESC RootSigDesc(NumOfParams, RootParameter,
			StaticSamplers.size(), StaticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		WRLComPtr<ID3DBlob> SerializedRootSig = nullptr;
		WRLComPtr<ID3DBlob> ErrorBlob = nullptr;
		HRESULT Hr = D3D12SerializeRootSignature(&RootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			SerializedRootSig.GetAddressOf(), ErrorBlob.GetAddressOf());

		if (ErrorBlob)
		{
			::OutputDebugStringA((char*)ErrorBlob->GetBufferPointer());
			std::cout << (char*)ErrorBlob->GetBufferPointer() << std::endl;
		}

		ThrowIfFailed(Hr);
		ThrowIfFailed(Device->CreateRootSignature(
			0,
			SerializedRootSig->GetBufferPointer(),
			SerializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&VoxelizeRootSignature)));
	}
	
	{
		// Resetcompute shader
		const int NumOfParams = 1; // 2 rw textures- albedo & normal
		CD3DX12_ROOT_PARAMETER RootParameter[NumOfParams];
		{
			CD3DX12_DESCRIPTOR_RANGE TexVoxelizer3d;
			TexVoxelizer3d.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0); // u0, u1
			RootParameter[0].InitAsDescriptorTable(1, &TexVoxelizer3d);
		}

		CD3DX12_ROOT_SIGNATURE_DESC RootSigDesc(NumOfParams, RootParameter, 0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		WRLComPtr<ID3DBlob> SerializedRootSig = nullptr;
		WRLComPtr<ID3DBlob> ErrorBlob = nullptr;
		HRESULT Hr = D3D12SerializeRootSignature(&RootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			SerializedRootSig.GetAddressOf(), ErrorBlob.GetAddressOf());

		if (ErrorBlob != nullptr)
		{
			::OutputDebugStringA((char*)ErrorBlob->GetBufferPointer());
		}
		ThrowIfFailed(Hr);

		ThrowIfFailed(Device->CreateRootSignature(
			0,
			SerializedRootSig->GetBufferPointer(),
			SerializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(VoxelizeResetRootSignature.GetAddressOf())));
	}
}

void MeshVoxelizer::BuildPSOs()
{
	WRLComPtr<ID3D12Device2> Device = Application::Get().GetDevice();
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC VoxelizePSODesc;
		ZeroMemory(&VoxelizePSODesc, sizeof(VoxelizePSODesc));

		VoxelizePSODesc.InputLayout.pInputElementDescs = Spec.InputLayout->data();
		VoxelizePSODesc.InputLayout.NumElements = static_cast<UINT>(Spec.InputLayout->size());

		VoxelizePSODesc.pRootSignature = VoxelizeRootSignature.Get();

		VoxelizePSODesc.VS.pShaderBytecode = (void*)VoxelizeVertexShader->GetByteCode()->GetBufferPointer();
		VoxelizePSODesc.VS.BytecodeLength = VoxelizeVertexShader->GetByteCode()->GetBufferSize();

		VoxelizePSODesc.PS.pShaderBytecode = (void*)VoxelizePixelShader->GetByteCode()->GetBufferPointer();
		VoxelizePSODesc.PS.BytecodeLength = VoxelizePixelShader->GetByteCode()->GetBufferSize();

		VoxelizePSODesc.GS.pShaderBytecode = (void*)VoxelizeGeoShader->GetByteCode()->GetBufferPointer();
		VoxelizePSODesc.GS.BytecodeLength = VoxelizeGeoShader->GetByteCode()->GetBufferSize();

		VoxelizePSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		VoxelizePSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		VoxelizePSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		VoxelizePSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		VoxelizePSODesc.DepthStencilState.DepthEnable = false; // Disable depth test
		VoxelizePSODesc.SampleMask = UINT_MAX;
		VoxelizePSODesc.NumRenderTargets = 0;
		VoxelizePSODesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;

		VoxelizePSODesc.SampleDesc.Count = 1;
		VoxelizePSODesc.SampleDesc.Quality = 0;
		VoxelizePSODesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

		ThrowIfFailed(Device->CreateGraphicsPipelineState(&VoxelizePSODesc, IID_PPV_ARGS(&VoxelizePSO)));
	}

	{
		// Compute PSO
		// ---------- Compute ------------- //
		D3D12_COMPUTE_PIPELINE_STATE_DESC ComputePSODesc = {};
		ComputePSODesc.pRootSignature = VoxelizeResetRootSignature.Get();
		ComputePSODesc.CS =
		{
			reinterpret_cast<BYTE*>(VoxelizerResetCS->GetByteCode()->GetBufferPointer()),
			VoxelizerResetCS->GetByteCode()->GetBufferSize()
		};

		ComputePSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		ThrowIfFailed(Device->CreateComputePipelineState(&ComputePSODesc, IID_PPV_ARGS(&VoxelizeResetPSO)));
	}
}

void MeshVoxelizer::BuildConstants()
{
	using namespace std;
	WRLComPtr<ID3D12Device2> Device = Application::Get().GetDevice();
	VoxelConstantUploader = make_unique< BufferUploader<VoxelizeContstants, true>>(Device.Get(), GetConstantBufferCount());

	const UINT HandleIncSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE Handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(Spec.ConstantBufferHeap->GetCPUDescriptorHandleForHeapStart());
	Handle.Offset(Spec.ConstBufferOffset, HandleIncSize);
	D3D12_CONSTANT_BUFFER_VIEW_DESC BufferDesc;
	BufferDesc.BufferLocation = VoxelConstantUploader->GetBuffer()->GetGPUVirtualAddress();
		//+ Spec.ConstBufferOffset * VoxelConstantUploader->GetElementSize();
	BufferDesc.SizeInBytes = VoxelConstantUploader->GetElementSize();
	Device->CreateConstantBufferView(&BufferDesc, Handle);
	/*
	const UINT HandleIncSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (size_t Idx = 0; Idx < Renderables.size(); Idx++)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE Handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(ConstantBufferHeap->GetCPUDescriptorHandleForHeapStart());
		Handle.Offset(Idx, HandleIncSize);
		D3D12_CONSTANT_BUFFER_VIEW_DESC BufferDesc;
		BufferDesc.BufferLocation = ObjectConstantBuffer->GetBuffer()->GetGPUVirtualAddress() 
			+ Idx * ObjectConstantBuffer->GetElementSize();
		BufferDesc.SizeInBytes = ObjectConstantBuffer->GetElementSize();
		Device->CreateConstantBufferView(&BufferDesc, Handle);
	}
	*/
	ConstantHandleGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(Spec.ConstantBufferHeap->GetGPUDescriptorHandleForHeapStart());
	ConstantHandleGPU.Offset(Spec.ConstBufferOffset, HandleIncSize);

	// Copy once?
	VoxelizeContstants Constants;
	Constants.WorldBound = Spec.SceneExtent;
	VoxelConstantUploader->CopyData(&Constants, 0);
}

void MeshVoxelizer::BuildSRVs()
{
	//using namespace std;
	WRLComPtr<ID3D12Device2> Device = Application::Get().GetDevice();
	const UINT HandleIncSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	{
		const INT HandleOffset = Spec.UAVSRVOffset;
		CD3DX12_CPU_DESCRIPTOR_HANDLE AlbedoUAVCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(Spec.UAVSRVHeap->GetCPUDescriptorHandleForHeapStart());
		AlbedoUAVCPU.Offset(HandleOffset, HandleIncSize);
		CD3DX12_GPU_DESCRIPTOR_HANDLE AlbedoUAVGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(Spec.UAVSRVHeap->GetGPUDescriptorHandleForHeapStart());
		AlbedoUAVGPU.Offset(HandleOffset, HandleIncSize);
		AlbedoTexture->SetupUAV(AlbedoUAVCPU, AlbedoUAVGPU);
	}

	{
		const INT HandleOffset = Spec.UAVSRVOffset + 1;
		CD3DX12_CPU_DESCRIPTOR_HANDLE NormalUAVCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(Spec.UAVSRVHeap->GetCPUDescriptorHandleForHeapStart());
		NormalUAVCPU.Offset(HandleOffset, HandleIncSize);
		CD3DX12_GPU_DESCRIPTOR_HANDLE NormalUAVGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(Spec.UAVSRVHeap->GetGPUDescriptorHandleForHeapStart());
		NormalUAVGPU.Offset(HandleOffset, HandleIncSize);
		NormalTexture->SetupUAV(NormalUAVCPU, NormalUAVGPU);
	}

	{
		const INT HandleOffset = Spec.UAVSRVOffset + 2;
		CD3DX12_CPU_DESCRIPTOR_HANDLE AlbedoSRVCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(Spec.UAVSRVHeap->GetCPUDescriptorHandleForHeapStart());
		AlbedoSRVCPU.Offset(HandleOffset, HandleIncSize);
		CD3DX12_GPU_DESCRIPTOR_HANDLE AlbedoSRVGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(Spec.UAVSRVHeap->GetGPUDescriptorHandleForHeapStart());
		AlbedoSRVGPU.Offset(HandleOffset, HandleIncSize);
		AlbedoTexture->SetupSRV(AlbedoSRVCPU, AlbedoSRVGPU);
	}

	{
		const INT HandleOffset = Spec.UAVSRVOffset + 3;
		CD3DX12_CPU_DESCRIPTOR_HANDLE NormalSRVCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(Spec.UAVSRVHeap->GetCPUDescriptorHandleForHeapStart());
		NormalSRVCPU.Offset(HandleOffset, HandleIncSize);
		CD3DX12_GPU_DESCRIPTOR_HANDLE NormalSRVGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(Spec.UAVSRVHeap->GetGPUDescriptorHandleForHeapStart());
		NormalSRVGPU.Offset(HandleOffset, HandleIncSize);
		NormalTexture->SetupSRV(NormalSRVCPU, NormalSRVGPU);
	}
}

void MeshVoxelizer::CreateShaders()
{
	using namespace std;
	// VertexShader = std::make_unique<Shader>(ResourceDirectory<std::wstring>::GetPath() + L"Shaders\\Meh.hlsl", 
	//nullptr, "VertexMain", "vs_5_0");
	// const std::wstring& Filename, const D3D_SHADER_MACRO* Defines, CRef<std::string> EntryPoint, CRef<std::string> Target
	VoxelizeVertexShader = make_unique<Shader>(ResourceDirectory<std::wstring>::GetPath() + L"Shaders\\MehVoxelize.hlsl",
		nullptr, "VertexMain", "vs_5_0");
	VoxelizeGeoShader = make_unique<Shader>(ResourceDirectory<std::wstring>::GetPath() + L"Shaders\\MehVoxelize.hlsl", 
		nullptr, "GeoMain", "gs_5_0");
	VoxelizePixelShader = make_unique<Shader>(ResourceDirectory<std::wstring>::GetPath() + L"Shaders\\MehVoxelize.hlsl",
		nullptr, "PixelMain", "ps_5_0");
	VoxelizerResetCS = make_unique<Shader>(ResourceDirectory<std::wstring>::GetPath() + L"Shaders\\MehVoxelize.hlsl",
		nullptr, "VoxelizeResetCS", "cs_5_0");
}
