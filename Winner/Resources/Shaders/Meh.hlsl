
Texture2D    gDiffuseMap : register(t0);
SamplerState gSampler  : register(s0);


cbuffer CBPerObject : register(b0)
{
	float4x4 gWorldViewProj;
}

struct VertexIn
{
	float3 PositionL : POSITION;
	float4 Color : COLOR;
	float2 Tex : TEX;
};

struct VertexOut
{
	float4 PositionH : SV_POSITION;
	float4 Color : COLOR;
	float2 Tex : TEX;
};

VertexOut VertexMain(VertexIn Vin)
{
	VertexOut Vout;

	// Transform to homogeneous clip space.
	Vout.PositionH = mul(float4(Vin.PositionL, 1.0f), gWorldViewProj);

	// Just pass vertex color into the pixel shader.
	Vout.Color = Vin.Color;
	Vout.Tex = Vin.Tex;

	return Vout;
}

float4 PixelMain(VertexOut Pin) : SV_Target
{
	//float4 diffuseAlbedo = gDiffuseMap.Sample(gSampler, float2(0.345, 0.2));
	float4 diffuseAlbedo = gDiffuseMap.Sample(gSampler, Pin.Tex);
	return diffuseAlbedo;
	return Pin.Color;
}