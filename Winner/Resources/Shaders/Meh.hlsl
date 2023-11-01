#pragma pack_matrix( row_major )

Texture2D    gDiffuseMap : register(t0);
SamplerState gSampler  : register(s0);


cbuffer CBPerObject : register(b0)
{
	float3 gLightDir;
	float padding;
	float4x4 gWorld;
	float4x4 gWorldViewProj;
}
 
struct VertexIn
{
	float3 PositionL : POSITION;
	float4 Color : COLOR;
	float3 Normal : NORMAL;
	float2 Tex : TEX;
};

struct VertexOut
{
	float4 PositionH : SV_POSITION;
	float4 Color : Mew;
	float3 Normal : Yo;
	float2 Tex : Mox;
};

VertexOut VertexMain(VertexIn Vin)
{
	VertexOut Vout;

	// Transform to homogeneous clip space.
	Vout.PositionH = mul(float4(Vin.PositionL, 1.0f), gWorldViewProj);

	// Just pass vertex color into the pixel shader.
	Vout.Color = Vin.Color;
	Vout.Tex = Vin.Tex;
	//Vout.Normal = Vin.Normal; // No non uniform scaling yet
	//Vout.Normal = normalize(mul(float4(Vin.Normal, 1.0f), gWorld).xyz);
	Vout.Normal = mul(Vin.Normal, (float3x3)gWorld);

	return Vout;
}

float4 PixelMain(VertexOut Pin) : SV_Target
{
	//float4 diffuseAlbedo = gDiffuseMap.Sample(gSampler, float2(0.345, 0.2));
	float4 DiffuseAlbedo = gDiffuseMap.Sample(gSampler, Pin.Tex);
	//return diffuseAlbedo;
	//return float4(abs(Pin.Normal.x), abs(Pin.Normal.y), abs(Pin.Normal.z), 1.f);
	//return float4(gLightDir, 1.f);
	//return Pin.Color;
	Pin.Normal = normalize(Pin.Normal);
	float3 LightDirW = normalize(gLightDir);
	float NDotL = max(dot(-LightDirW, Pin.Normal), 0.f);
	//return float4(NDotL, NDotL, NDotL, 1.f);
	//return float4(Pin.Normal, 1.f);
	return DiffuseAlbedo * NDotL;
}