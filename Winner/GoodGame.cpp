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
	/*
	DirectX::TexMetadata Metadata;
	DirectX::ScratchImage ScratchImage;

	DirectX::LoadFromDDSFile(L"", 0, &Metadata, ScratchImage);
	*/
	BoxTexture = std::make_unique<Texture>();
	//MehTexture->Load(L"", )

	if (!bLoadedContent)
	{
		std::shared_ptr<CommandQueue> CommandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
		WRLComPtr<ID3D12GraphicsCommandList2> CommandList = CommandQueue->GetCommandList();
		Microsoft::WRL::ComPtr<ID3D12Device2> Device = Application::Get().GetDevice();

		BoxTexture->Load(ResourceDirectory<std::wstring>::GetPath() + L"Textures\\Directx9.png", Device.Get(), CommandList.Get());

		BuildDescriptorHeaps(CommandList.Get());
		BuildConstantBuffers(CommandList.Get());
		BuildRootSignature(CommandList.Get());
		BuildShadersAndInputLayout(CommandList.Get());
		BuildBoxGeometry(CommandList.Get());
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

void GoodGame::OnUpdate(UpdateEventArgs & e)
{
	Game::OnUpdate(e);
	using namespace DirectX;
	// Build the view matrix.
	DirectX::XMVECTOR pos = DirectX::XMVectorSet(5.f, 6.f, 3.f, 1.f);
	DirectX::XMVECTOR target = DirectX::XMVectorZero();
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&ViewMat, view);

	XMMATRIX world = XMLoadFloat4x4(&WorldMat);
	XMMATRIX proj = XMLoadFloat4x4(&ProjectionMat);

	XMMATRIX worldViewProj = world * view * proj;

	// Update the constant buffer with the latest worldViewProj matrix.
	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	ObjectConstantBuffer->CopyData(&objConstants, 0);
}

void GoodGame::OnRender(RenderEventArgs & e)
{
	Game::OnRender(e);

	auto CommandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	WRLComPtr<ID3D12GraphicsCommandList2> CommandList = CommandQueue->GetCommandList();

	UINT currentBackBufferIndex = m_pWindow->GetCurrentBackBufferIndex();
	Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer = m_pWindow->GetCurrentBackBuffer();
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_pWindow->GetCurrentRenderTargetView();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();

	// Clear the render targets.
	{
		CD3DX12_RESOURCE_BARRIER Yobarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		
		CommandList->ResourceBarrier(1, &Yobarrier);
		//TransitionResource(commandList, backBuffer,
		//	D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		FLOAT clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };

		//ClearRTV(commandList, rtv, clearColor);
		CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
		//ClearDepth(commandList, dsv);
		CommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
	}

	CommandList->SetPipelineState(PSO.Get());
	CommandList->SetGraphicsRootSignature(RootSignature.Get());

	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	CommandList->IASetVertexBuffers(0, 1, &BoxGeometry->GetVertexBufferView());
	CommandList->IASetIndexBuffer(&BoxGeometry->GetIndexBufferView());



	CommandList->RSSetViewports(1, &Viewport);
	CommandList->RSSetScissorRects(1, &ScissorRect);

	CommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	ID3D12DescriptorHeap* descriptorHeaps[] = { ConstantBufferHeap.Get() };
	CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	CommandList->SetGraphicsRootDescriptorTable(0, ConstantBufferHeap->GetGPUDescriptorHandleForHeapStart());
	
	ID3D12DescriptorHeap* descriptorHeaps2[] = { SrvDescriptorHeap.Get() };
	CommandList->SetDescriptorHeaps(_countof(descriptorHeaps2), descriptorHeaps2);
	CommandList->SetGraphicsRootDescriptorTable(1, SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	const UINT YoIndexCount = BoxGeometry->DrawArgs["box"].IndexCount;
	
	CommandList->DrawIndexedInstanced(YoIndexCount, 1, 0, 0, 0);

	// Present
	{
		CD3DX12_RESOURCE_BARRIER Yobarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), 
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);

		CommandList->ResourceBarrier(1, &Yobarrier);

		m_FenceValues[currentBackBufferIndex] = CommandQueue->ExecuteCommandList(CommandList);

		currentBackBufferIndex = m_pWindow->Present();

		CommandQueue->WaitForFenceValue(m_FenceValues[currentBackBufferIndex]);
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
	//Game::OnResize(e);

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
	CbvHeapDesc.NumDescriptors = 1;
	CbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	CbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	CbvHeapDesc.NodeMask = 0;

	WRLComPtr<ID3D12Device2> Device = Application::Get().GetDevice();

	ThrowIfFailed(Device->CreateDescriptorHeap(&CbvHeapDesc,
		IID_PPV_ARGS(&ConstantBufferHeap)) );

	// Shader resource view...
	D3D12_DESCRIPTOR_HEAP_DESC SrvHeapDesc = {};
	SrvHeapDesc.NumDescriptors = 1;
	SrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	SrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(Device->CreateDescriptorHeap(&SrvHeapDesc, IID_PPV_ARGS(SrvDescriptorHeap.GetAddressOf())));

	CD3DX12_CPU_DESCRIPTOR_HANDLE TexDescriptor(SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	//std::unique_ptr<Texture>& Stonk = mTextures["woodCrateTex"];
	ID3D12Resource* woodCrateTex = BoxTexture->GetDefaultBuffer(); //mTextures["woodCrateTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
	SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.Format = woodCrateTex->GetDesc().Format;
	SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	SrvDesc.Texture2D.MostDetailedMip = 0;
	SrvDesc.Texture2D.MipLevels = BoxTexture->GetAvailableMipLevels();//1;// woodCrateTex->GetDesc().MipLevels;
	SrvDesc.Texture2D.ResourceMinLODClamp = 0;

	Device->CreateShaderResourceView(woodCrateTex, &SrvDesc, TexDescriptor);
}

void GoodGame::BuildConstantBuffers(ID3D12GraphicsCommandList2* CommandList)
{
	WRLComPtr<ID3D12Device2> Device = Application::Get().GetDevice();
	ObjectConstantBuffer = std::make_unique< BufferUploader<ObjectConstants, 1, true>>(Device.Get());
	
	D3D12_CONSTANT_BUFFER_VIEW_DESC BufferDesc;
	BufferDesc.BufferLocation = ObjectConstantBuffer->GetBuffer()->GetGPUVirtualAddress();
	BufferDesc.SizeInBytes = ObjectConstantBuffer->GetElementSize();

	Device->CreateConstantBufferView(&BufferDesc, ConstantBufferHeap->GetCPUDescriptorHandleForHeapStart());
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
}

void GoodGame::BuildShadersAndInputLayout(ID3D12GraphicsCommandList2* CommandList)
{
	VertexShader = std::make_unique<Shader>(ResourceDirectory<std::wstring>::GetPath() + L"Shaders\\Meh.hlsl", 
		nullptr, "VertexMain", "vs_5_0");
	PixelShader = std::make_unique<Shader>(ResourceDirectory<std::wstring>::GetPath() + L"Shaders\\Meh.hlsl", 
		nullptr, "PixelMain", "ps_5_0");

	InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12 + 4 * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void GoodGame::BuildBoxGeometry(ID3D12GraphicsCommandList2* CommandList)
{
	using namespace DirectX;
	using namespace std;
	float w2 = 1;
	float h2 = 1;
	float d2 = 1;
	array<Vertex, 24> Vertices =
	{
		/*
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White), XMFLOAT2(0.f, 1.f) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black), XMFLOAT2(0.f, 0.f) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red), XMFLOAT2(1.f, 0.f) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green), XMFLOAT2(1.f, 1.f) }),

		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue), XMFLOAT2(0.f, 1.f) }), 
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow), XMFLOAT2(0.f, 0.f) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan), XMFLOAT2(1.f, 0.f) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta), XMFLOAT2(1.f, 1.f) })
		*/
		// Fill in the front face vertex data.
		Vertex( -w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f),
	Vertex(-w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f),
	Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f),
	Vertex(+w2, -h2, -d2,  1.0f, 0.0f, 0.0f, 1.0f, 1.0f),

	// Fill in the back face vertex data.
	Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f),
	Vertex(+w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f),
	Vertex(+w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f),
	Vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f),

	// Fill in the top face vertex data.
	Vertex(-w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f),
	Vertex(-w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f),
	Vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f),
	Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f),

	// Fill in the bottom face vertex data.
	Vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f),
	Vertex(+w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f),
	Vertex(+w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f),
	Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f),

	// Fill in the left face vertex data.
	Vertex(-w2, -h2, +d2, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f),
	Vertex(-w2, +h2, +d2, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
	Vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),
	Vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f),

	// Fill in the right face vertex data.
	Vertex(+w2, -h2, -d2, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f),
	Vertex(+w2, +h2, -d2, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f),
	Vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f),
	Vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f)
	};

	

	uint16_t i[36];

	// Fill in the front face index data
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// Fill in the back face index data
	i[6] = 4; i[7] = 5; i[8] = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	// Fill in the top face index data
	i[12] = 8; i[13] = 9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// Fill in the bottom face index data
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	// Fill in the left face index data
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	// Fill in the right face index data
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

	const UINT VertexBufferSize = Vertices.size() * sizeof(Vertex);
	const UINT IndexBufferSize = 36 * sizeof(std::uint16_t);

	BoxGeometry = std::make_unique<MeshGeometry>();
	BoxGeometry->Name = "boxGeometry";

	ThrowIfFailed(D3DCreateBlob(VertexBufferSize, &BoxGeometry->VertexBufferCPU));
	CopyMemory(BoxGeometry->VertexBufferCPU->GetBufferPointer(), Vertices.data(), VertexBufferSize);

	ThrowIfFailed(D3DCreateBlob(IndexBufferSize, &BoxGeometry->IndexBufferCPU));
	CopyMemory(BoxGeometry->IndexBufferCPU->GetBufferPointer(), i, IndexBufferSize);

	WRLComPtr<ID3D12Device2> Device = Application::Get().GetDevice();
	//std::shared_ptr<CommandQueue> CommandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	//Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CommandList = CommandQueue->GetCommandList();

	BoxGeometry->VertexBufferUploader = make_unique<DefaultBufferUploader>();
	BoxGeometry->VertexBufferUploader->CreateAndUpload(Device.Get(), CommandList, Vertices.data(), VertexBufferSize);

	BoxGeometry->IndexBufferUploader = make_unique<DefaultBufferUploader>();
	BoxGeometry->IndexBufferUploader->CreateAndUpload(Device.Get(), CommandList, i, IndexBufferSize);
	
	BoxGeometry->VertexByteStride = sizeof(Vertex);
	BoxGeometry->VertexBufferByteSize = VertexBufferSize;
	BoxGeometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	BoxGeometry->IndexBufferByteSize = IndexBufferSize;
	
	SubmeshGeometry Submesh;
	Submesh.IndexCount = static_cast<UINT>(36);
	Submesh.StartIndexLocation = 0;
	Submesh.BaseVertexLocation = 0;

	BoxGeometry->DrawArgs["box"] = Submesh;
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