#include "Common.hlsl"

Texture2D DiffuseTexture : register(t0);
SamplerState DiffuseSampler : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
};

PSInput VS(VSInput input)
{
    PSInput output;
    output.position = ApplyMVP(input.position);
    output.texCoord = input.texCoord;
    output.color = PrimitiveColor;
    return output;
}

float4 PS(PSInput input) : SV_TARGET
{
    float4 baseColor = input.color;
    if (StaticMeshUseTexture != 0)
    {
        baseColor *= DiffuseTexture.Sample(DiffuseSampler, input.texCoord);
    }

    return lerp(baseColor, float4(WireframeRGB, 1.0f), bIsWireframe);
}
