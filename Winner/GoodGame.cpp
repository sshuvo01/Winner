#include "GoodGame.h"
#include "Mesh.h"
#include "UploadHelpers.h"
#include "Application.h"
#include "CommandQueue.h"
#include "Window.h"

#include <DirectXTex/DirectXTex/DirectXTex.h>
#include <d3dcompiler.h>

#include <DirectXColors.h>
#include <algorithm> // For std::min and std::max.
#include <sstream>
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

GoodGame::GoodGame(const std::wstring & Name, int Width, int Height, bool VSync)
	: Game(Name, Width, Height, VSync),
	bLoadedContent { false },
	Viewport{ CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(Width), static_cast<float>(Height)) },
	ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
{
}

bool GoodGame::LoadContent()
{
	using namespace std;

	if (!bLoadedContent)
	{
		std::shared_ptr<CommandQueue> CommandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
		WRLComPtr<ID3D12GraphicsCommandList2> CommandList = CommandQueue->GetCommandList();
		Microsoft::WRL::ComPtr<ID3D12Device2> Device = Application::Get().GetDevice();

		unique_ptr<Texture> Texture1 = std::make_unique<Texture>();
		Texture1->Load(ResourceDirectory<std::wstring>::GetPath() + L"Textures\\Me.jpg", Device.Get(), CommandList.Get());

		unique_ptr<Texture> Texture2 = std::make_unique<Texture>();
		Texture2->Load(ResourceDirectory<std::wstring>::GetPath() + L"Textures\\Directx9.png", Device.Get(), CommandList.Get());

		ComputeInput = std::make_unique<RenderTexture>();
		ComputeOutput = std::make_unique<RenderTexture>();

		Textures.push_back(move(Texture1));
		Textures.push_back(move(Texture2));

		BuildGeometry(CommandList.Get());
		BuildDescriptorHeaps(CommandList.Get());
		BuildShaderResrources(CommandList.Get());
		BuildConstantBuffers(CommandList.Get());
		BuildRootSignature(CommandList.Get());
		BuildShadersAndInputLayout(CommandList.Get());
		BuildPSO(CommandList.Get());

		bLoadedContent = true; // ?

		// Create the descriptor heap for the depth-stencil view.
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(Application::Get().GetDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap)));

		/*
		Before leaving the LoadContent method, the command list must be executed on the command queue
		to ensure that the index and vertex buffers are uploaded to the GPU resources before rendering.
		*/
		uint64_t fenceValue = CommandQueue->ExecuteCommandList(CommandList);
		CommandQueue->WaitForFenceValue(fenceValue);

		ResizeDepthBuffer(GetClientWidth(), GetClientHeight());
	}
	return true;
}

void GoodGame::UnloadContent()
{
}

//template <template<class, class> class V, class T, class A>
//static void ftemplatetemplate(V<T, A> &v) {
//	// This can be "typename V<T, A>::value_type",
//	// but we are pretending we don't have it
//
//	//T temp = v.back();
//	//v.pop_back();
//	v.push_back(T());
//	T a;
//	a.foo();
//	// Do some work on temp
//	
//	//std::cout << temp << std::endl;
//}

void GoodGame::OnUpdate(UpdateEventArgs & e)
{
	Game::OnUpdate(e);
	using namespace DirectX;
	using namespace std;

	// Build the view matrix.
	DirectX::XMVECTOR pos = DirectX::XMVectorSet(5.f, 6.f, 3.f, 1.f);
	DirectX::XMVECTOR target = DirectX::XMVectorZero();
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX View = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&ViewMat, View);
	XMMATRIX proj = XMLoadFloat4x4(&ProjectionMat);

	DirectX::XMFLOAT3 LightDirection = { -2.0f, -3.707f, -1.707f };
	for (const auto& Rable : Renderables)
	{
		Rable->Update(e);
		XMMATRIX WorldViewProj = Rable->WorldMat * View * proj;
		ObjectConstants ObjConstants;
		XMStoreFloat4x4(&ObjConstants.WorldViewProj, WorldViewProj);
		XMStoreFloat4x4(&ObjConstants.World, Rable->WorldMat);
		ObjConstants.LightDir = LightDirection;
		ObjectConstantBuffer->CopyData(&ObjConstants, Rable->HeapIndexMap[ConstantStr]);
	}

	// Update the constant buffer with the latest worldViewProj matrix.
	//objConstants.LightDir = { -2.0f, -3.707f, -1.707f };
	
	// Another box
	/*ObjectConstants objConstants2 = objConstants;
	XMMATRIX world2 = XMMatrixTranslation(0.f, 2.1f, 0.f);
	XMMATRIX worldViewProj2 = world2 * view * proj;*/
	
	//XMMATRIX World2T = XMMatrixTranspose(world2);

	//XMStoreFloat4x4(&objConstants2.WorldViewProj, worldViewProj2);
	//XMStoreFloat4x4(&objConstants2.World, world2);

	////world = XMMatrixRotationX(e.TotalTime) * XMMatrixTranslation(std::sin(e.TotalTime), 0.f, std::sin(e.TotalTime));
	//ObjectConstantBuffer->CopyData(&objConstants2, 1);
	//
	//// Plane constants
	//ObjectConstants objConstants3 = objConstants;
	//XMMATRIX World3 =  XMMatrixScaling(10.f, 10.f, 10.f) * XMMatrixTranslation(0.f, -5.f, 0.f);
	//XMMATRIX WorldViewProj3 = World3 * view * proj;
	//
	//XMStoreFloat4x4(&objConstants3.WorldViewProj, WorldViewProj3);
	//XMStoreFloat4x4(&objConstants3.World, World3);
	//ObjectConstantBuffer->CopyData(&objConstants3, 2);


	//std::stringstream SS;
	//std::string sss;
	//SS << "This is a test " << 2 << " " << 2.132f << std::endl;
	//std::string Str = SS.str();
	//::OutputDebugStringA(Str.c_str());

	//Useful::FormattedString YoString("test ", 2, " ", 3.1416f, " meh");
	//::OutputDebugStringA(YoString.GetCStr());
	//YoString.GetFormattedDebugString("test", 2, 2.132f);
	/*class Test
	{
	public:
		void foo() {};
	};
	std::vector<Test> saf;
	ftemplatetemplate(saf);*/
}

void GoodGame::OnRender(RenderEventArgs & e)
{
	//std::cout << "On Render" << std::endl;
	Game::OnRender(e);

	auto CommandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	WRLComPtr<ID3D12GraphicsCommandList2> CommandList = CommandQueue->GetCommandList();

	UINT CurrentBackBufferIndex = m_pWindow->GetCurrentBackBufferIndex();
	Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer = m_pWindow->GetCurrentBackBuffer();
	D3D12_CPU_DESCRIPTOR_HANDLE Rtv = m_pWindow->GetCurrentRenderTargetView();
	D3D12_CPU_DESCRIPTOR_HANDLE Dsv = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();

	// Clear the render targets.
	{
		CD3DX12_RESOURCE_BARRIER RTBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		
		CommandList->ResourceBarrier(1, &RTBarrier);
		FLOAT clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
		CommandList->ClearRenderTargetView(Rtv, clearColor, 0, nullptr);
		CommandList->ClearDepthStencilView(Dsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
	}

	CommandList->SetPipelineState(PSO.Get());
	CommandList->SetGraphicsRootSignature(RootSignature.Get());

	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	CommandList->RSSetViewports(1, &Viewport);
	CommandList->RSSetScissorRects(1, &ScissorRect);
	CommandList->OMSetRenderTargets(1, &Rtv, FALSE, &Dsv);

	for (const auto& Rable : Renderables)
	{
		CommandList->IASetVertexBuffers(0, 1, &Rable->MeshGeo->GetVertexBufferView());
		CommandList->IASetIndexBuffer(&Rable->MeshGeo->GetIndexBufferView());
		
		ID3D12DescriptorHeap* ConstantHeaps[] = { ConstantBufferHeap.Get() };
		CommandList->SetDescriptorHeaps(_countof(ConstantHeaps), ConstantHeaps);
		CD3DX12_GPU_DESCRIPTOR_HANDLE CBHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(ConstantBufferHeap->GetGPUDescriptorHandleForHeapStart());
		CBHandle.Offset(Rable->HeapIndexMap[ConstantStr],
			Application::Get().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
		CommandList->SetGraphicsRootDescriptorTable(0, CBHandle);
		
		ID3D12DescriptorHeap** ShaderResourceHeap = SrvDescriptorHeap.GetAddressOf();
		CommandList->SetDescriptorHeaps(1, ShaderResourceHeap);
		CD3DX12_GPU_DESCRIPTOR_HANDLE SrvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		SrvHandle.Offset(Rable->HeapIndexMap[TextureStr],
			Application::Get().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
		CommandList->SetGraphicsRootDescriptorTable(1, SrvHandle);
		
		const UINT IndexCount = Rable->MeshGeo->DrawArgs["default"].IndexCount;
		CommandList->DrawIndexedInstanced(IndexCount, 1, 0, 0, 0);
	}


	{
		CD3DX12_RESOURCE_BARRIER Yobarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COPY_SOURCE);
		CommandList->ResourceBarrier(1, &Yobarrier);

		CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ComputeInput->GetResource().Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

		CommandList->CopyResource(ComputeInput->GetResource().Get(), backBuffer.Get());
		
		CommandList->SetPipelineState(ComputePSO.Get());
		CommandList->SetComputeRootSignature(ComputeRootSignature.Get());
		ID3D12DescriptorHeap* ShaderResourceHeap[] = 
		{ 
			RenderTexture::DescHeaps[ (UINT) RenderTexture::Specification::Type::ShaderResource ].Get() 
		};
		
		CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ComputeInput->GetResource().Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

		CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ComputeOutput->GetResource().Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

		CommandList->SetDescriptorHeaps(_countof(ShaderResourceHeap), ShaderResourceHeap);
		CommandList->SetComputeRootDescriptorTable(0, ComputeInput->GetGPUHandle());

		ID3D12DescriptorHeap* UAHeap[] =
		{
			RenderTexture::DescHeaps[(UINT)RenderTexture::Specification::Type::UnorderedAccess].Get()
		};

		CommandList->SetDescriptorHeaps(_countof(UAHeap), UAHeap);
		CommandList->SetComputeRootDescriptorTable(1, ComputeOutput->GetGPUHandle());

		const UINT NumGroupsX = (UINT)ceilf((float) GetClientWidth() / 256.0f);
		CommandList->Dispatch(NumGroupsX, GetClientHeight(), 1);

		CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ComputeOutput->GetResource().Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));


		CD3DX12_RESOURCE_BARRIER Yobarrier2 = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_COPY_DEST);
		CommandList->ResourceBarrier(1, &Yobarrier2);

		CommandList->CopyResource(backBuffer.Get(), ComputeOutput->GetResource().Get());

		CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ComputeInput->GetResource().Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COMMON));

		CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ComputeOutput->GetResource().Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COMMON));
	}
	
	// Present
	{
		CD3DX12_RESOURCE_BARRIER Yobarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), 
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PRESENT);

		CommandList->ResourceBarrier(1, &Yobarrier);
		FenceValues[CurrentBackBufferIndex] = CommandQueue->ExecuteCommandList(CommandList);
		CurrentBackBufferIndex = m_pWindow->Present();
		CommandQueue->WaitForFenceValue(FenceValues[CurrentBackBufferIndex]);
	}
}

void GoodGame::OnKeyPressed(KeyEventArgs & e)
{
	Game::OnKeyPressed(e);
}

void GoodGame::OnMouseWheel(MouseWheelEventArgs & e)
{
	Game::OnMouseWheel(e);
}

void GoodGame::OnResize(ResizeEventArgs & e)
{
	if (e.Width != GetClientWidth() || e.Height != GetClientHeight())
	{
		Game::OnResize(e);

		Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
			static_cast<float>(e.Width), static_cast<float>(e.Height));

		ResizeDepthBuffer(e.Width, e.Height);
	}
}

void GoodGame::BuildDescriptorHeaps(ID3D12GraphicsCommandList2* CommandList)
{
	D3D12_DESCRIPTOR_HEAP_DESC CbvHeapDesc;
	CbvHeapDesc.NumDescriptors = 3;
	CbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	CbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	CbvHeapDesc.NodeMask = 0;

	WRLComPtr<ID3D12Device2> Device = Application::Get().GetDevice();
	ThrowIfFailed(Device->CreateDescriptorHeap(&CbvHeapDesc,
		IID_PPV_ARGS(&ConstantBufferHeap)) );

	// Shader resource view... textures
	D3D12_DESCRIPTOR_HEAP_DESC SrvHeapDesc = {};
	SrvHeapDesc.NumDescriptors = Textures.size();
	SrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	SrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(Device->CreateDescriptorHeap(&SrvHeapDesc, IID_PPV_ARGS(SrvDescriptorHeap.GetAddressOf())));
}

void GoodGame::BuildShaderResrources(ID3D12GraphicsCommandList2* CommandList)
{
	WRLComPtr<ID3D12Device2> Device = Application::Get().GetDevice();
	const UINT HandleIncSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (size_t Idx = 0; Idx < Textures.size(); Idx++)
	{
		const auto& Tex = Textures[Idx];
		CD3DX12_CPU_DESCRIPTOR_HANDLE TexHandle(SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		TexHandle.Offset(Idx, HandleIncSize);
		ID3D12Resource* TexResource = Tex->GetDefaultBuffer();
		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
		SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SrvDesc.Format = TexResource->GetDesc().Format;
		SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SrvDesc.Texture2D.MostDetailedMip = 0;
		SrvDesc.Texture2D.MipLevels = Tex->GetAvailableMipLevels();
		SrvDesc.Texture2D.ResourceMinLODClamp = 0;
		Device->CreateShaderResourceView(TexResource, &SrvDesc, TexHandle);
	}
	// Compute shader input
	RenderTexture::Specification ComputeInputSpec;
	ComputeInputSpec.Type_ = RenderTexture::Specification::Type::ShaderResource;
	ComputeInputSpec.Width = GetClientWidth();
	ComputeInputSpec.Height = GetClientHeight();
	ComputeInputSpec.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ComputeInput->BuildResource(ComputeInputSpec, nullptr);
	ComputeInput->BuildDescriptor();
	// Compute shader output
	RenderTexture::Specification ComputeOutputSpec;
	ComputeOutputSpec.Type_ = RenderTexture::Specification::Type::UnorderedAccess;
	ComputeOutputSpec.Width = GetClientWidth();
	ComputeOutputSpec.Height = GetClientHeight();
	ComputeOutputSpec.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ComputeOutput->BuildResource(ComputeOutputSpec, nullptr);
	ComputeOutput->BuildDescriptor();
}

void GoodGame::BuildConstantBuffers(ID3D12GraphicsCommandList2* CommandList)
{
	WRLComPtr<ID3D12Device2> Device = Application::Get().GetDevice();
	ObjectConstantBuffer = std::make_unique< BufferUploader<ObjectConstants, true>>(Device.Get(), Renderables.size());

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
}

void GoodGame::BuildRootSignature(ID3D12GraphicsCommandList2* CommandList)
{
	CD3DX12_ROOT_PARAMETER RootParameter[2];

	// Create a single descriptor table of CBVs
	CD3DX12_DESCRIPTOR_RANGE CbvTable;
	CbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	RootParameter[0].InitAsDescriptorTable(1, &CbvTable);
	
	CD3DX12_DESCRIPTOR_RANGE TexTable;
	TexTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	RootParameter[1].InitAsDescriptorTable(1, &TexTable, D3D12_SHADER_VISIBILITY_PIXEL);
	/*
		UINT numParameters,
        _In_reads_opt_(numParameters) const D3D12_ROOT_PARAMETER* _pParameters,
        UINT numStaticSamplers = 0,
        _In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC* _pStaticSamplers = NULL,
        D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE)
	*/
	auto StaticSamp = GetStaticSamplers();
	CD3DX12_ROOT_SIGNATURE_DESC RootSigDesc(2, RootParameter, 
		StaticSamp.size(), StaticSamp.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	WRLComPtr<ID3DBlob> SerializedRootSig = nullptr;
	WRLComPtr<ID3DBlob> ErrorBlob = nullptr;
	HRESULT Hr = D3D12SerializeRootSignature(&RootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		SerializedRootSig.GetAddressOf(), ErrorBlob.GetAddressOf());

	if (ErrorBlob)
	{
		::OutputDebugStringA((char*)ErrorBlob->GetBufferPointer());
	}

	ThrowIfFailed(Hr);
	WRLComPtr<ID3D12Device2> Device = Application::Get().GetDevice();
	ThrowIfFailed(Device->CreateRootSignature(
		0,
		SerializedRootSig->GetBufferPointer(),
		SerializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&RootSignature)));

	BuildComputeRootSignature(CommandList);
}

void GoodGame::BuildComputeRootSignature(ID3D12GraphicsCommandList2 * CommandList)
{
	WRLComPtr<ID3D12Device2> Device = Application::Get().GetDevice();
	CD3DX12_DESCRIPTOR_RANGE SrvTable;
	SrvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE UavTable;
	UavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

	CD3DX12_ROOT_PARAMETER SlotRootParameter[2];

	SlotRootParameter[0].InitAsDescriptorTable(1, &SrvTable);
	SlotRootParameter[1].InitAsDescriptorTable(1, &UavTable);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC RootSigDesc(2, SlotRootParameter,
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// Create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	WRLComPtr<ID3DBlob> SerializedRootSig = nullptr;
	WRLComPtr<ID3DBlob> ErrorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&RootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		SerializedRootSig.GetAddressOf(), ErrorBlob.GetAddressOf());

	if (ErrorBlob != nullptr)
	{
		::OutputDebugStringA((char*)ErrorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(Device->CreateRootSignature(
		0,
		SerializedRootSig->GetBufferPointer(),
		SerializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(ComputeRootSignature.GetAddressOf())));
}

void GoodGame::BuildShadersAndInputLayout(ID3D12GraphicsCommandList2* CommandList)
{
	VertexShader = std::make_unique<Shader>(ResourceDirectory<std::wstring>::GetPath() + L"Shaders\\Meh.hlsl", 
		nullptr, "VertexMain", "vs_5_0");
	PixelShader = std::make_unique<Shader>(ResourceDirectory<std::wstring>::GetPath() + L"Shaders\\Meh.hlsl", 
		nullptr, "PixelMain", "ps_5_0");
	ComputeShader = std::make_unique<Shader>(ResourceDirectory<std::wstring>::GetPath() + L"Shaders\\MehCompute.hlsl",
		nullptr, "PostProcessCS", "cs_5_0");

	InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 3 * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, (3 + 4) * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0  },
		{ "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, (3 + 4 + 3) * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void GoodGame::BuildGeometry(ID3D12GraphicsCommandList2* CommandList)
{
	using namespace DirectX;
	using namespace std;
	unique_ptr<Renderable> RableBox1 = make_unique<Renderable>();
	unique_ptr<Renderable> RableBox2 = make_unique<Renderable>();
	unique_ptr<Renderable> RablePlane = make_unique<Renderable>();

	RableBox1->SetUpdateCallback([RableRaw = RableBox1.get()](const UpdateEventArgs & EventArgs)
	{
		RableRaw->WorldMat = XMMatrixRotationX(EventArgs.TotalTime) * XMMatrixRotationY(std::sin(EventArgs.TotalTime))
			* XMMatrixRotationZ(std::cos(EventArgs.TotalTime)) * XMMatrixTranslation(0.f, -3.f, 0.f);
	});

	RableBox2->SetUpdateCallback([RableRaw = RableBox2.get()](const UpdateEventArgs & EventArgs)
	{
		RableRaw->WorldMat = XMMatrixTranslation(0.f, 2.1f, 0.f);
	});

	RABLEUPDATECALLBACK(RablePlane,
	{
		Obj->WorldMat = XMMatrixScaling(10.f, 10.f, 10.f) * XMMatrixTranslation(0.f, -5.f, 0.f);
	});

	// Box!
	BoxMeshData BoxMesh;
	const UINT VertexBufferSize = BoxMesh.GetVerticesByteSize();
	const UINT IndexBufferSize = BoxMesh.GetIndicesByteSize();

	shared_ptr<MeshGeometry> BoxGeometry = make_shared<MeshGeometry>();
	BoxGeometry->Name = "boxGeometry";

	ThrowIfFailed(D3DCreateBlob(VertexBufferSize, &BoxGeometry->VertexBufferCPU));
	::memcpy(BoxGeometry->VertexBufferCPU->GetBufferPointer(), BoxMesh.GetVertices()->data(), VertexBufferSize);

	ThrowIfFailed(D3DCreateBlob(IndexBufferSize, &BoxGeometry->IndexBufferCPU));
	::memcpy(BoxGeometry->IndexBufferCPU->GetBufferPointer(), BoxMesh.GetIndices()->data(), IndexBufferSize);

	WRLComPtr<ID3D12Device2> Device = Application::Get().GetDevice();
	
	BoxGeometry->VertexBufferUploader = make_unique<DefaultBufferUploader>();
	BoxGeometry->VertexBufferUploader->CreateAndUpload(Device.Get(), CommandList, BoxMesh.GetVertices()->data(), VertexBufferSize);

	BoxGeometry->IndexBufferUploader = make_unique<DefaultBufferUploader>();
	BoxGeometry->IndexBufferUploader->CreateAndUpload(Device.Get(), CommandList, BoxMesh.GetIndices()->data(), IndexBufferSize);
	
	BoxGeometry->VertexByteStride = BoxMesh.GetVertexByteStride();
	BoxGeometry->VertexBufferByteSize = VertexBufferSize;
	BoxGeometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	BoxGeometry->IndexBufferByteSize = IndexBufferSize;
	
	SubmeshGeometry Submesh;
	Submesh.IndexCount = (UINT)BoxMesh.GetIndices()->size();
	Submesh.StartIndexLocation = 0;
	Submesh.BaseVertexLocation = 0;

	BoxGeometry->DrawArgs["default"] = Submesh;

	RableBox1->MeshGeo = BoxGeometry;
	RableBox2->MeshGeo = move(BoxGeometry);
	//static const string ConstantStr = "Constant";
	//static const string TextureStr = "Texture";
	RableBox1->HeapIndexMap[ConstantStr] = 0;
	RableBox1->HeapIndexMap[TextureStr] = 0;
	
	RableBox2->HeapIndexMap[ConstantStr] = 1;
	RableBox2->HeapIndexMap[TextureStr] = 0;

	//Rable->MeshGeo
	// Plane!
	PlaneMeshData PlaneMesh;
	shared_ptr<MeshGeometry> PlaneGeometry = make_unique<MeshGeometry>();
	PlaneGeometry->Name = "planeGeometry";

	ThrowIfFailed(D3DCreateBlob(PlaneMesh.GetVerticesByteSize(), &PlaneGeometry->VertexBufferCPU));
	::memcpy(PlaneGeometry->VertexBufferCPU->GetBufferPointer(), PlaneMesh.GetVertices()->data(), PlaneMesh.GetVerticesByteSize());

	ThrowIfFailed(D3DCreateBlob(PlaneMesh.GetIndicesByteSize(), &PlaneGeometry->IndexBufferCPU));
	::memcpy(PlaneGeometry->IndexBufferCPU->GetBufferPointer(), PlaneMesh.GetIndices()->data(), PlaneMesh.GetIndicesByteSize());
	
	PlaneGeometry->VertexBufferUploader = make_unique<DefaultBufferUploader>();
	PlaneGeometry->VertexBufferUploader->CreateAndUpload(Device.Get(), CommandList, PlaneMesh.GetVertices()->data(), PlaneMesh.GetVerticesByteSize());
	
	PlaneGeometry->IndexBufferUploader = make_unique<DefaultBufferUploader>();
	PlaneGeometry->IndexBufferUploader->CreateAndUpload(Device.Get(), CommandList, PlaneMesh.GetIndices()->data(), PlaneMesh.GetIndicesByteSize());

	PlaneGeometry->VertexByteStride = PlaneMesh.GetVertexByteStride();
	PlaneGeometry->VertexBufferByteSize = PlaneMesh.GetVerticesByteSize();
	PlaneGeometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	PlaneGeometry->IndexBufferByteSize = PlaneMesh.GetIndicesByteSize();

	SubmeshGeometry Submesh2;
	Submesh2.IndexCount = (UINT)PlaneMesh.GetIndices()->size();
	Submesh2.StartIndexLocation = 0;
	Submesh2.BaseVertexLocation = 0;

	PlaneGeometry->DrawArgs["default"] = Submesh2;

	RablePlane->MeshGeo = move(PlaneGeometry);
	RablePlane->HeapIndexMap[ConstantStr] = 2;
	RablePlane->HeapIndexMap[TextureStr] = 1;
	// Add them to the list
	Renderables.push_back(move(RableBox1));
	Renderables.push_back(move(RableBox2));
	Renderables.push_back(move(RablePlane));
}

void GoodGame::BuildPSO(ID3D12GraphicsCommandList2* CommandList)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
	ZeroMemory(&PSODesc, sizeof(PSODesc));
	
	PSODesc.InputLayout.pInputElementDescs = InputLayout.data();
	PSODesc.InputLayout.NumElements = static_cast<UINT>(InputLayout.size());
	
	PSODesc.pRootSignature = RootSignature.Get();

	PSODesc.VS.pShaderBytecode = (void*)VertexShader->GetByteCode()->GetBufferPointer();
	PSODesc.VS.BytecodeLength = VertexShader->GetByteCode()->GetBufferSize();

	PSODesc.PS.pShaderBytecode = (void*)PixelShader->GetByteCode()->GetBufferPointer();
	PSODesc.PS.BytecodeLength = PixelShader->GetByteCode()->GetBufferSize();

	PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	PSODesc.SampleMask = UINT_MAX;
	PSODesc.NumRenderTargets = 1;
	PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	
	PSODesc.SampleDesc.Count = 1;
	PSODesc.SampleDesc.Quality = 0;
	PSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	WRLComPtr<ID3D12Device2> Device = Application::Get().GetDevice();
	ThrowIfFailed(Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO)));

	// ---------- Compute ------------- //
	D3D12_COMPUTE_PIPELINE_STATE_DESC ComputePSODesc = {};
	ComputePSODesc.pRootSignature = ComputeRootSignature.Get();
	ComputePSODesc.CS =
	{
		reinterpret_cast<BYTE*>(ComputeShader->GetByteCode()->GetBufferPointer()),
		ComputeShader->GetByteCode()->GetBufferSize()
	};

	ComputePSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(Device->CreateComputePipelineState(&ComputePSODesc, IID_PPV_ARGS(&ComputePSO)));
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GoodGame::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}

void GoodGame::ResizeDepthBuffer(int Width, int Height)
{
	if (bLoadedContent)
	{
		// Flush any GPU commands that might be referencing the depth buffer.
		Application::Get().Flush();

		Width = std::max(1, Width);
		Height = std::max(1, Height);

		auto device = Application::Get().GetDevice();

		// Resize screen dependent resources.
		// Create a depth buffer.
		D3D12_CLEAR_VALUE optimizedClearValue = {};
		optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		optimizedClearValue.DepthStencil = { 1.0f, 0 };

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, Width, Height,
				1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&optimizedClearValue,
			IID_PPV_ARGS(&m_DepthBuffer)
		));

		// Update the depth-stencil view.
		D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
		dsv.Format = DXGI_FORMAT_D32_FLOAT;
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv.Texture2D.MipSlice = 0;
		dsv.Flags = D3D12_DSV_FLAG_NONE;

		device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsv,
			m_DSVHeap->GetCPUDescriptorHandleForHeapStart());
	}

	const float AR = static_cast<float>(GetClientWidth()) / static_cast<float>(GetClientHeight());
	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * Useful::PI, AR, 1.0f, 1000.0f);
	XMStoreFloat4x4(&ProjectionMat, P);
}