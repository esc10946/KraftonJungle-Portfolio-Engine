Texture2D SceneTexture : register(t0);
Texture2D MaskTexture : register(t1);
SamplerState ViewSampler : register(s0);

cbuffer OutlineCompositeCB : register(b0)
{
    float4 CompositeOutlineColor;
    float2 CompositeTexelSize;
    float CompositeOutlineThickness;
    float CompositePadding;
};

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

PSInput VS(VSInput input)
{
    PSInput output;
    output.position = float4(input.position, 1.0f);
    output.texCoord = input.texCoord;
    return output;
}

float SampleMask(float2 uv)
{
    return MaskTexture.Sample(ViewSampler, uv).r;
}

float4 PS(PSInput input) : SV_TARGET
{
    float4 sceneColor = SceneTexture.Sample(ViewSampler, input.texCoord);

    float2 offsetX = float2(CompositeTexelSize.x * CompositeOutlineThickness, 0.0f);
    float2 offsetY = float2(0.0f, CompositeTexelSize.y * CompositeOutlineThickness);

    float c = SampleMask(input.texCoord);
    float l = SampleMask(input.texCoord - offsetX);
    float r = SampleMask(input.texCoord + offsetX);
    float u = SampleMask(input.texCoord - offsetY);
    float d = SampleMask(input.texCoord + offsetY);

    float neighborMax = max(max(l, r), max(u, d));
    float edge = (c < 0.5f && neighborMax > 0.5f) ? 1.0f : 0.0f;

    return lerp(sceneColor, CompositeOutlineColor, edge);
}