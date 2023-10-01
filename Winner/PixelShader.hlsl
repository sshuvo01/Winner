/*
*/
struct PixelShaderInput
{
	float4 Color    : COLOR;
};

float4 main( PixelShaderInput IN ) : SV_Target
{
    return IN.Color;
   // return float4(1.f, 0.f, 0.f, 1.f);
}