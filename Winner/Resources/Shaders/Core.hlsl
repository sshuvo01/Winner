#ifndef __CORE_HLSL__
#define __CORE_HLSL__

cbuffer CBPerObject : register(b0)
{
    float3 gLightDir; // TODO: this is not per object, move this some place else
    float padding;
    float4x4 gWorld; 
    float4x4 gWorldViewProj;
}

cbuffer CBPerFrame : register(b1)
{
    // float3 gLightDir;
    // float padding1;
    // float4x4 gView
    // float4x4 gProj
}

cbuffer CBVoxelizerContstant : register(b2)
{
    float gWorldBound;
}

SamplerState gSampler : register(s0);

uint ConvVec4ToRGBA8(float4 val)
{
    return (uint(val.w) & 0x000000FF) << 24U | (uint(val.z) & 0x000000FF) << 16U | (uint(val.y) & 0x000000FF) << 8U | (uint(val.x) & 0x000000FF);
}


#endif // __CORE_HLSL__