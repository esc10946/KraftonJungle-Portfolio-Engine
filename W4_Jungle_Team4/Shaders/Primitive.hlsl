#include "Common.hlsl"

struct VSInput
{
    float3 position : POSITION; // Input position from vertex buffer
    float4 color : COLOR; // Input color from vertex buffer
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VS(VSInput input)
{
    PSInput output;
    output.position = ApplyMVP(input.position);
    output.color = input.color;
    return output;
}

float4 PS(PSInput input) : SV_TARGET
{
    return lerp(input.color, float4(WireframeRGB, 1.0f), bIsWireframe);
}