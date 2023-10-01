#pragma once
#include <vector>
#include <array>

#include "Game.h"
#include "Core.h"
#include "Shader.h"
#include "Mesh.h"
#include "Window.h"
#include "UploadHelpers.h"

struct Vertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT4 Color;
};

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 WorldViewProj = Useful::Identity4x4();
};

class GoodGame : public Game
{
public:
	GoodGame(const std::wstring& Name, int Width, int Height, bool VSync);
	GoodGame() = delete;
	~GoodGame() {}

	/**
	 *  Load content required for the demo.
	 */
	virtual bool LoadContent() override;

	/**
	 *  Unload demo specific content that was loaded in LoadContent.
	 */
	virtual void UnloadContent() override;

protected:

	virtual void OnUpdate(UpdateEventArgs& e) override;

	/**
	 *  Render stuff.
	 */
	virtual void OnRender(RenderEventArgs& e) override;

	/**
	 * Invoked by the registered window when a key is pressed
	 * while the window has focus.
	 */
	virtual void OnKeyPressed(KeyEventArgs& e) override;

	/**
	 * Invoked when the mouse wheel is scrolled while the registered window has focus.
	 */
	virtual void OnMouseWheel(MouseWheelEventArgs& e) override;


	virtual void OnResize(ResizeEventArgs& e) override;

private:
	DirectX::XMFLOAT4X4 WorldMat = Useful::Identity4x4();
	DirectX::XMFLOAT4X4 ViewMat = Useful::Identity4x4();
	DirectX::XMFLOAT4X4 ProjectionMat = Useful::Identity4x4();

	D3D12_VIEWPORT Viewport;
	D3D12_RECT ScissorRect;

	bool bLoadedContent;

	void BuildDescriptorHeaps(ID3D12GraphicsCommandList2* CommandList);
	void BuildConstantBuffers(ID3D12GraphicsCommandList2* CommandList);
	void BuildRootSignature(ID3D12GraphicsCommandList2* CommandList);
	void BuildShadersAndInputLayout(ID3D12GraphicsCommandList2* CommandList);
	void BuildBoxGeometry(ID3D12GraphicsCommandList2* CommandList);
	void BuildPSO(ID3D12GraphicsCommandList2* CommandList);

	WRLComPtr<ID3D12RootSignature> RootSignature;
	WRLComPtr<ID3D12DescriptorHeap> ConstantBufferHeap;
	WRLComPtr<ID3D12PipelineState> PSO;
	std::unique_ptr < BufferUploader<ObjectConstants, 1, true> > ObjectConstantBuffer;
	std::unique_ptr< Shader > VertexShader, PixelShader;

	std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout;
	std::unique_ptr<MeshGeometry> BoxGeometry;

	void ResizeDepthBuffer(int Width, int Height);

	// Depth buffer.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuffer;
	// Descriptor heap for depth buffer.
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSVHeap;

	uint64_t m_FenceValues[Window::BufferCount] = {};
};

