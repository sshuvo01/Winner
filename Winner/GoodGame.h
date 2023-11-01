#pragma once
#include <vector>
#include <array>

#include "Game.h"
#include "Core.h"
#include "Shader.h"
#include "Mesh.h"
#include "Window.h"
#include "UploadHelpers.h"
#include "Texture.h"
#include "Renderable.h"

struct ObjectConstants
{
	DirectX::XMFLOAT3 LightDir; float Padding;
	DirectX::XMFLOAT4X4 World = Useful::Identity4x4();
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
	void BuildShaderResrources(ID3D12GraphicsCommandList2* CommandList);
	void BuildConstantBuffers(ID3D12GraphicsCommandList2* CommandList);
	void BuildRootSignature(ID3D12GraphicsCommandList2* CommandList);
	void BuildShadersAndInputLayout(ID3D12GraphicsCommandList2* CommandList);
	void BuildGeometry(ID3D12GraphicsCommandList2* CommandList);
	void BuildPSO(ID3D12GraphicsCommandList2* CommandList);
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

	WRLComPtr<ID3D12RootSignature> RootSignature;
	WRLComPtr<ID3D12PipelineState> PSO;
	std::unique_ptr < BufferUploader<ObjectConstants, true> > ObjectConstantBuffer;
	std::unique_ptr< Shader > VertexShader, PixelShader;

	std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout;
	//std::shared_ptr<MeshGeometry> BoxGeometry;
	//std::shared_ptr<MeshGeometry> PlaneGeometry;

	void ResizeDepthBuffer(int Width, int Height);

	// Depth buffer.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuffer;
	// Descriptor heap for depth buffer.
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSVHeap;
	WRLComPtr<ID3D12DescriptorHeap> ConstantBufferHeap;
	WRLComPtr<ID3D12DescriptorHeap> SrvDescriptorHeap;

	uint64_t FenceValues[Window::BufferCount] = {};
	//std::unique_ptr<Texture> BoxTexture;
	//std::unique_ptr<Texture> AnotherTexture;

	std::vector<std::unique_ptr<Renderable>> Renderables;
	std::vector<std::unique_ptr<Texture>> Textures;

	static constexpr const char* ConstantStr = "Constant";
	static constexpr const char* TextureStr = "Texture";
};

