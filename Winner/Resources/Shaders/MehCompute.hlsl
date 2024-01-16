Texture2D gInput            : register(t0);
RWTexture2D<float4> gOutput : register(u0);

#define N 256

[numthreads(N, 1, 1)]
void PostProcessCS(int3 DispatchThreadID : SV_DispatchThreadID)
{
	float StepSize = 8.0;
	float4 InputColor = gInput[DispatchThreadID.xy];
	InputColor.rgb = round(InputColor.rgb * StepSize) / StepSize;
	gOutput[DispatchThreadID.xy] = InputColor;
}
