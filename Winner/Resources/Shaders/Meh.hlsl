

cbuffer CBPerObject : register(b0)
{
	float4x4 gWorldViewProj;
}

struct VertexIn
{
	float3 PositionL : POSITION;
	float4 Color : COLOR;
};

struct VertexOut
{
	float4 PositionH : SV_POSITION;
	float4 Color : COLOR;
};

VertexOut VertexMain(VertexIn Vin)
{
	VertexOut Vout;

	// Transform to homogeneous clip space.
	Vout.PositionH = mul(float4(Vin.PositionL, 1.0f), gWorldViewProj);

	// Just pass vertex color into the pixel shader.
	Vout.Color = Vin.Color;

	return Vout;
}

float4 PixelMain(VertexOut Pin) : SV_Target
{
	return Pin.Color;
}