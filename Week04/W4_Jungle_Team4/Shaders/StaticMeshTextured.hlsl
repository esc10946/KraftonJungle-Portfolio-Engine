#include "Common.hlsl"

Texture2D DiffuseTexture : register(t0);
SamplerState DiffuseSampler : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
    float3 worldPos : TEXCOORD1;
};

PSInput VS(VSInput input)
{
    PSInput output;
    float4 worldPos = mul(float4(input.position, 1.0f), Model);
    output.position = mul(mul(worldPos, View), Projection);
    output.normal = normalize(mul(float4(input.normal, 0.0f), Model).xyz);
    output.texCoord = input.texCoord;
    output.color = PrimitiveColor;
    output.worldPos = worldPos.xyz;
    return output;
}

float2 ApplyAtlasUV(float2 uv, float3 normal)
{
    if (StaticMeshSourceUVsUseAtlasTiles != 0)
    {
        uv = frac(uv * StaticMeshSourceAtlasTileScale);
    }

    float4 targetRect = StaticMeshSideUVRect;

    if (normal.z > 0.5f)
    {
        targetRect = StaticMeshTopUVRect;
    }
    else if (normal.z < -0.5f)
    {
        targetRect = StaticMeshBottomUVRect;
    }

    return float2(
        targetRect.x + uv.x * (targetRect.z - targetRect.x),
        targetRect.w - uv.y * (targetRect.w - targetRect.y));
}

float4 PS(PSInput input) : SV_TARGET
{
    float4 baseColor = input.color;
    float2 sampleUV = input.texCoord * UVTiling + UVOffset + UVScrollSpeed * GlobalTime;
    if (StaticMeshUseTexture != 0)
    {
        if (StaticMeshUseAtlas != 0)
        {
            sampleUV = ApplyAtlasUV(sampleUV, input.normal);
        }

        baseColor *= DiffuseTexture.Sample(DiffuseSampler, sampleUV);
        
    }
    baseColor.a *= PrimitiveColor.a;
    return lerp(baseColor, float4(WireframeRGB, 1.0f), bIsWireframe);
}
