#pragma once
#include "Core.h"
#include <unordered_map>
#include <memory>
// Defines a subrange of geometry in a MeshGeometry.  This is for when multiple
// geometries are stored in one vertex and index buffer.  It provides the offsets
// and data needed to draw a subset of geometry stores in the vertex and index 
// buffers so that we can implement the technique described by Figure 6.3.
struct SubmeshGeometry
{
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;
};

class MeshGeometry : public NonCopyable
{
public:
	std::string Name;

	WRLComPtr<ID3DBlob> VertexBufferCPU;
	WRLComPtr<ID3DBlob> IndexBufferCPU;

	std::unique_ptr<class DefaultBufferUploader> IndexBufferUploader;
	std::unique_ptr<DefaultBufferUploader> VertexBufferUploader;

	// Data about the buffers.
	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
	UINT IndexBufferByteSize = 0;

	// A MeshGeometry may store multiple geometries in one vertex/index buffer.
	// Use this container to define the Submesh geometries so we can draw
	// the Submeshes individually.
	std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const;
};

class MeshData
{
public:
	using VerticesType = std::vector<float>;
	using IndicesType = std::vector<UINT16>;
	enum class DataLayout
	{
		Pos3Col3Nor3Tex2,
		Pos3Col4Nor3Tex2,
		Pos3Nor3Tex2,
		Unknown
	};

	FORCEINLINE const VerticesType* GetVertices() const
	{
		return &Vertices;
	}

	FORCEINLINE const IndicesType* GetIndices() const
	{
		return &Indices;
	}
	
	FORCEINLINE UINT64 GetVerticesByteSize() const
	{
		return GetVertices()->size() * sizeof(VerticesType::value_type);
	}

	FORCEINLINE UINT64 GetIndicesByteSize() const
	{
		return GetIndices()->size() * sizeof(IndicesType::value_type);
	}

	FORCEINLINE DataLayout GetLayout() const
	{
		return Layout;
	}

	virtual ~MeshData() { }
	virtual UINT64 GetVertexByteStride() const = 0;

protected:
	VerticesType Vertices;
	IndicesType Indices;
	DataLayout Layout = DataLayout::Unknown;

	MeshData() {}
};

class BoxMeshData final : public MeshData
{
public:
	BoxMeshData();
	~BoxMeshData();

	virtual UINT64 GetVertexByteStride() const override;
};

class PlaneMeshData final : public MeshData
{
public:
	PlaneMeshData();
	~PlaneMeshData();

	virtual UINT64 GetVertexByteStride() const override;
};