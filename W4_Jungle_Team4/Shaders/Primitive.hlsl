#include "Common.hlsl"

struct VSInput
{
    float3 position : POSITION; // Input position from vertex buffer
    float3 normal : NORMAL;
    float4 color : COLOR; // Input color from vertex buffer
    float2 textCoord : TEXTURE;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 textCoord : TEXTURE;
};

PSInput VS(VSInput input)
{
    PSInput output;
    output.position = ApplyMVP(input.position);
    output.color = input.color;
    return output;
}

Texture2D g_txColor : register(t0);
SamplerState g_Sample : register(s0);

float4 PS(PSInput input) : SV_TARGET
{
    float4 col = g_txColor.Sample(g_Sample, input.textCoord);
    if (bIsWireframe < 0.5f)
    {
        if (col.r < 0.1f)
        {
            discard; // 검은 배경 제거    
        }
        return col;
    }
    
    return float4(WireframeRGB, 1.0f);
    
    return lerp(input.color, float4(WireframeRGB, 1.0f), bIsWireframe);
}