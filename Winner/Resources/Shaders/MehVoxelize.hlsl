#pragma pack_matrix( row_major )
#define GS_MAX_VERTEX 3
#include "Core.hlsl"

Texture2D    gDiffuseMap : register(t0);

RWTexture3D<uint> gVoxelizerAlbedo : register(u0);
RWTexture3D<uint> gVoxelizerNormal : register(u1);

struct VertexIn
{
	float3 PositionL : POSITION;
	float4 Color : COLOR;
	float3 Normal : NORMAL;
	float2 Tex : TEX;
};

struct GeomIn
{
    float3 PositionW : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float2 Tex : TEX;
};

struct PixelIn
{
    float4 PositionH : SV_POSITION;
    float3 PositionW : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float2 Tex : TEX;
};

GeomIn VertexMain(VertexIn Vin)
{
    GeomIn Gin;
    Gin.PositionW = mul(float4(Vin.PositionL, 1.f), gWorld).xyz;
    Gin.Color = Vin.Color;
    Gin.Normal = normalize(mul(Vin.Normal, (float3x3) gWorld));
    Gin.Tex = Vin.Tex;
    
    return Gin;
}

[maxvertexcount(GS_MAX_VERTEX)]
void GeoMain(triangle GeomIn Gins[GS_MAX_VERTEX], inout TriangleStream<PixelIn> TriStreamOut)
{
    PixelIn Pin;
    //float3 PosW0 = Gins[0].PositionW;
    //float3 PosW1 = Gins[1].PositionW;
    //float3 PosW2 = Gins[2].PositionW;
    //float3 Normal = cross(normalize(posW1 - posW0), normalize(posW2 - posW0));
    // TODO: is this right?
    float3 Normal = normalize((Gins[0].Normal + Gins[1].Normal + Gins[2].Normal) / 3.f);
    
    float AxisDots[] =
    {
        abs(dot(Normal, float3(1.f, 0.f, 0.f))), // X
        abs(dot(Normal, float3(0.f, 1.f, 0.f))),
        abs(dot(Normal, float3(0.f, 0.f, 1.f)))
    };
    
    int MaxAxisDotIndex = 0, Iter;
    for (Iter = 1; Iter < GS_MAX_VERTEX; Iter++)
    {
        if (AxisDots[MaxAxisDotIndex] < AxisDots[Iter])
        {
            MaxAxisDotIndex = Iter;
        }
    }
    
    
    for (Iter = 0; Iter < GS_MAX_VERTEX; Iter++)
    {
        float4 OutputPos;

        switch (MaxAxisDotIndex)
        {
            case 0:
                OutputPos = float4(Gins[Iter].PositionW.yzx, 1.0f);
                break;
            case 1:
                OutputPos = float4(Gins[Iter].PositionW.zxy, 1.0f);
                break;
            case 2:
                OutputPos = float4(Gins[Iter].PositionW.xyz, 1.0f);
                break;
        }

        OutputPos.z = 0.0;
	 
        Pin.Tex = Gins[Iter].Tex;
        Pin.PositionW = Gins[Iter].PositionW / (gWorldBound * 0.5f); // TODO: find out what this magic value is!, half bound? make it a constant!
        Pin.PositionH = mul(OutputPos, gWorldViewProj);
        Pin.Normal = Gins[Iter].Normal;
        Pin.Color = Gins[Iter].Color;
        
        TriStreamOut.Append(Pin);
    }
    
}

void PixelMain(PixelIn Pin)
{
   // Pin.Normal = normalize(Pin.Normal);
   // float4 Albedo = gDiffuseMap.Sample(gSampler, Pin.Tex);
    
    uint3 TexDimension;
    gVoxelizerAlbedo.GetDimensions(TexDimension.x, TexDimension.y, TexDimension.z);
    
    uint3 TexIndex = uint3(((Pin.PositionW.x * 0.5) + 0.5f) * TexDimension.x,
		((Pin.PositionW.y * 0.5) + 0.5f) * TexDimension.y,
		((Pin.PositionW.z * 0.5) + 0.5f) * TexDimension.z);
    
    if(all(TexIndex >= 0) && all(TexIndex < TexDimension))
    {  
        float4 DiffuseAlbedo = gDiffuseMap.Sample(gSampler, Pin.Tex);
        float Opacity = DiffuseAlbedo.a;

        if (Opacity > 0.0)
        {
            Pin.Normal = normalize(Pin.Normal);
            gVoxelizerAlbedo[TexIndex] = ConvVec4ToRGBA8(DiffuseAlbedo * 255.0f);
            gVoxelizerNormal[TexIndex] = ConvVec4ToRGBA8(float4((Pin.Normal.xyz / 2.0 + float3(0.5, 0.5, 0.5)), 1.0) * 255.0f);
        }
    }
}

[numthreads(8, 8, 8)]
void VoxelizeResetCS(int3 DispatchThreadID : SV_DispatchThreadID)
{
    int X = DispatchThreadID.x;
    int Y = DispatchThreadID.y;
    int Z = DispatchThreadID.z;
	 
    gVoxelizerAlbedo[int3(X, Y, Z)] = 0;
    gVoxelizerNormal[int3(X, Y, Z)] = 0;
}