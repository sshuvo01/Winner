#include "Mesh.h"
#include "UploadHelpers.h"

D3D12_VERTEX_BUFFER_VIEW MeshGeometry::GetVertexBufferView() const
{
	if (!VertexBufferUploader)
	{
		ASSERTNOENTRY("No!");
		return D3D12_VERTEX_BUFFER_VIEW();
	}

	D3D12_VERTEX_BUFFER_VIEW Vbv;
	Vbv.BufferLocation = VertexBufferUploader->GetDefaultBuffer()->GetGPUVirtualAddress();
	Vbv.StrideInBytes = VertexByteStride;
	Vbv.SizeInBytes = VertexBufferByteSize;

	return Vbv;
}

D3D12_INDEX_BUFFER_VIEW MeshGeometry::GetIndexBufferView() const
{
	if (!IndexBufferUploader)
	{
		ASSERTNOENTRY("No!");
		return D3D12_INDEX_BUFFER_VIEW();
	}

	D3D12_INDEX_BUFFER_VIEW Ibv;
	Ibv.BufferLocation = IndexBufferUploader->GetDefaultBuffer()->GetGPUVirtualAddress();
	Ibv.Format = IndexFormat;
	Ibv.SizeInBytes = IndexBufferByteSize;

	return Ibv;
}
