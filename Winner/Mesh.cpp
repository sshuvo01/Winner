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

BoxMeshData::BoxMeshData()
{
	Layout = DataLayout::Pos3Col3Nor3Tex2;

	Vertices = 
	{
			/* Pos */			/* Color */			  /* Normal */		   /* Tex */
		// Fill in the front face vertex data.
		-1.f, -1.f, -1.f,   1.0f, 0.0f, 0.0f, 1.f,	 0.0f, 0.0f, -1.0f,    0.0f, 1.0f,
		-1.f, +1.f, -1.f,   1.0f, 1.0f, 0.0f, 1.f,   0.0f, 0.0f, -1.0f,    0.0f, 0.0f,
		 1.f, 1.f, -1.f,    1.0f, 0.0f, 0.0f, 1.f,   0.0f, 0.0f, -1.0f,    1.0f, 0.0f,
		 1.f, -1.f, -1.f,   0.1f, 0.3f, 1.0f, 1.f,   0.0f, 0.0f, -1.0f,    1.0f, 1.0f,

		 // Fill in the back face vertex data.
		-1.f, -1.f, 1.f,    1.0f, 0.0f, 0.0f, 1.f,   0.0f, 0.0f, 1.0f,     1.0f, 1.0f,
		 1.f, -1.f, 1.f,    0.2f, 0.3f, 0.5f, 1.f,   0.0f, 0.0f, 1.0f,     0.0f, 1.0f,
		 1.f, 1.f, 1.f,     1.0f, 0.0f, 0.0f, 1.f,   0.0f, 0.0f, 1.0f,     0.0f, 0.0f,
		-1.f, 1.f, 1.f,     1.0f, 1.0f, 0.0f, 1.f,   0.0f, 0.0f, 1.0f,     1.0f, 0.0f,

		// Fill in the top face vertex data.
		-1.f, 1.f, -1.f,    0.0f, 1.0f, 1.0f, 1.f,   0.0f, 1.0f, 0.0f,     0.0f, 1.0f,
		-1.f, 1.f, 1.f,     1.0f, 0.0f, 0.0f, 1.f,   0.0f, 1.0f, 0.0f,     0.0f, 0.0f,
		 1.f, 1.f, 1.f,     1.0f, 1.0f, 1.0f, 1.f,   0.0f, 1.0f, 0.0f,     1.0f, 0.0f,
		 1.f, 1.f, -1.f,    1.0f, 0.0f, 1.0f, 1.f,   0.0f, 1.0f, 0.0f,     1.0f, 1.0f,

		 // Fill in the bottom face vertex data.
		-1.f, -1.f, -1.f,   1.0f, 0.0f, 0.0f, 1.f,   0.0f, -1.0f, 0.0f,    1.0f, 1.0f,
		 1.f, -1.f, -1.f,   0.8f, 1.0f, 0.3f, 1.f,   0.0f, -1.0f, 0.0f,    0.0f, 1.0f,
		 1.f, -1.f,  1.f,   0.0f, 1.0f, 0.0f, 1.f,   0.0f, -1.0f, 0.0f,    0.0f, 0.0f,
		-1.f, -1.f, 1.f,    1.0f, 1.0f, 1.0f, 1.f,   0.0f, -1.0f, 0.0f,    1.0f, 0.0f,

		// Fill in the left face vertex data.
		-1.f, -1.f, 1.f,    0.0f, 0.0f, 1.0f, 1.f,  -1.0f, 0.0f, 0.0f,     0.0f, 1.0f,
		-1.f, 1.f, 1.f,     0.0f, 0.0f, 1.0f, 1.f,  -1.0f, 0.0f, 0.0f,     0.0f, 0.0f,
		-1.f, 1.f, -1.f,    0.1f, 1.0f, 1.0f, 1.f,  -1.0f, 0.0f, 0.0f,     1.0f, 0.0f,
		-1.f, -1.f, -1.f,   0.0f, 1.0f, 0.0f, 1.f,  -1.0f, 0.0f, 0.0f,     1.0f, 1.0f,

		// Fill in the right face vertex data.
		1.f, -1.f, -1.f,    0.0f, 0.0f, 1.0f, 1.f,   1.0f, 0.0f, 0.0f,     0.0f, 1.0f,
		1.f, 1.f, -1.f,     1.0f, 0.0f, 1.0f, 1.f,   1.0f, 0.0f, 0.0f,     0.0f, 0.0f,
		1.f, 1.f, 1.f,      1.0f, 0.2f, 1.0f, 1.f,   1.0f, 0.0f, 0.0f,     1.0f, 0.0f,
		1.f, -1.f, 1.f,     0.0f, 1.0f, 1.0f, 1.f,   1.0f, 0.0f, 0.0f,     1.0f, 1.0f
	};

	Indices.resize(36);
	// Fill in the front face index data
	Indices[0] = 0; Indices[1] = 1; Indices[2] = 2;
	Indices[3] = 0; Indices[4] = 2; Indices[5] = 3;

	// Fill in the back face index data
	Indices[6] = 4; Indices[7] = 5; Indices[8] = 6;
	Indices[9] = 4; Indices[10] = 6; Indices[11] = 7;

	// Fill in the top face index data
	Indices[12] = 8; Indices[13] = 9; Indices[14] = 10;
	Indices[15] = 8; Indices[16] = 10; Indices[17] = 11;

	// Fill in the bottom face index data
	Indices[18] = 12; Indices[19] = 13; Indices[20] = 14;
	Indices[21] = 12; Indices[22] = 14; Indices[23] = 15;

	// Fill in the left face index data
	Indices[24] = 16; Indices[25] = 17; Indices[26] = 18;
	Indices[27] = 16; Indices[28] = 18; Indices[29] = 19;

	// Fill in the right face index data
	Indices[30] = 20; Indices[31] = 21; Indices[32] = 22;
	Indices[33] = 20; Indices[34] = 22; Indices[35] = 23;
}

BoxMeshData::~BoxMeshData()
{
}

UINT64 BoxMeshData::GetVertexByteStride() const
{
	return 12 * sizeof(VerticesType::value_type);
}

PlaneMeshData::PlaneMeshData()
{
	//ASSERTNOENTRY("Not yet!");
	Layout = DataLayout::Pos3Col3Nor3Tex2;

	Vertices = 
	{
		   /* Pos */			/* Color */			  /* Normal */		   /* Tex */
		-1.f, 0.f, -1.f,   0.0f, 0.0f, 1.0f, 1.f,   0.0f, 1.0f, 0.0f,    1.0f, 1.0f,
		 1.f, 0.f, -1.f,   0.8f, 0.0f, 0.3f, 1.f,   0.0f, 1.0f, 0.0f,    0.0f, 1.0f,
		 1.f, 0.f,  1.f,   0.4f, 1.0f, 1.0f, 1.f,   0.0f, 1.0f, 0.0f,    0.0f, 0.0f,
		-1.f, 0.f, 1.f,    1.0f, 0.0f, 1.0f, 1.f,   0.0f, 1.0f, 0.0f,    1.0f, 0.0f,
	};

	Indices =
	{
		0, 2, 1,
		0, 3, 2
	};
}

PlaneMeshData::~PlaneMeshData()
{
}

UINT64 PlaneMeshData::GetVertexByteStride() const
{
	return 12 * sizeof(VerticesType::value_type);
}
