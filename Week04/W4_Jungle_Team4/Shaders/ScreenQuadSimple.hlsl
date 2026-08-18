#include "Common.hlsl"

struct VSInput
{
	float3 position : POSITION;
	float4 color : COLOR;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

PSInput VS(VSInput input)
{
	PSInput output;
	output.position = float4(input.position, 1.0);
	output.color = input.color;
	return output;
}

float4 PS(PSInput input) : SV_TARGET
{
	return input.color;
}
