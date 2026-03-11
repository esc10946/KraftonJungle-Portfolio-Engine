// ShaderW0.hlsl
cbuffer constants : register(b0)
{
    float3 Offset;
    float WipeProgress;
    float4 Scale;
    //float Pad;
    float4 BlockColor;
}

struct VS_INPUT
{
    float4 position : POSITION; // Input position from vertex buffer
    float4 color : COLOR; // Input color from vertex buffer  
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // Transformed position to pass to the pixel shader
    float4 color : COLOR; // Color to pass to the pixel shader
    float localX : TEXCOORD0;
    float localY : TEXCOORD1;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    // Pass the position directly to the pixel shader (no transformation)
    // output.position = input.position;
    
    // 상수버퍼를 통해 넘겨 받은 Offset을 더해서 버텍스를 이동 시켜 픽셀쉐이더로 넘김
    // output.position = float4(Offset, 0) + input.position;
    
    // 상수버퍼를 통해 넘겨 받은 Offset과 Scale 값을 바탕으로 Vertex들의 위치를 이동
    float3 scaledPos = input.position.xyz * Scale.xyz;
    float3 finalPos = scaledPos + Offset.xyz;
    output.position = float4(finalPos, input.position.w);
    output.localX = input.position.x;
    output.localY = input.position.y;
    // Pass the color to the pixel shader
    output.color = input.color;
    
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 borderColor = float4(input.color.rgb, 1) * BlockColor;
    float4 fillColor = float4(input.color.rgb, 1) * BlockColor; 
    float4 baseColor = lerp(borderColor, fillColor, input.color.a);

    if (WipeProgress > -1.5f)
    {
        float diagonalPos = input.localX + input.localY;
        
        float sweep = saturate(1.0f - abs(diagonalPos - WipeProgress) / 0.1f);
        return lerp(baseColor, float4(1, 1, 1, 1), sweep);
    }
    return baseColor;
}