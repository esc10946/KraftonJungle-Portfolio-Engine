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
    float4 worldPos = mul(float4(input.position, 1.0f), Model);
    output.position = mul(mul(worldPos, View), Projection);
    output.color = PrimitiveColor;
    return output;
}

float4 PS(PSInput input) : SV_TARGET
{
    return lerp(input.color, float4(WireframeRGB, 1.0f), bIsWireframe);
}
