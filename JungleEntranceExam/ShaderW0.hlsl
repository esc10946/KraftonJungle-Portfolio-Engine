cbuffer constants : register(b0)
{
    float4 Offset;
    float4 Scale;
    float Alpha;
    float3 Padding;
};

struct VS_INPUT
{
    float4 position : POSITION;
    float4 color : COLOR;
    
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    float3 scaledPos = input.position.xyz * Scale.xyz;
    float3 finalPos = scaledPos + Offset.xyz;
    
    output.position = float4(finalPos, input.position.w);
    output.color = input.color;
    output.color.a = Alpha;
    
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    return input.color;
}