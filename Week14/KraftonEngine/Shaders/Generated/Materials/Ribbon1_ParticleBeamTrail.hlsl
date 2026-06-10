// Generated from Content/Material/Particle/Ribbon1.uasset
// Domain: ParticleBeamTrail

#include "Common/ConstantBuffers.hlsli"
#include "Common/VertexLayouts.hlsli"
#include "Common/Functions.hlsli"
#include "Common/SystemSamplers.hlsli"
#define USE_FOG 1
#include "Common/Fog.hlsli"

cbuffer ForwardFogParams : register(b7)
{
    float4 FwdFogColor;
    float  FwdFogDensity;
    float  FwdFogHeightFalloff;
    float  FwdFogBaseHeight;
    float  FwdFogStartDistance;
    float  FwdFogCutoffDistance;
    float  FwdFogMaxOpacity;
    float2 _fwdFogPad;
};

float4 ApplyFogTransparent(float4 color, float3 worldPos, float3 cameraWorldPos)
{
    float fogFactor = ComputeHeightFogFactor(
        worldPos, cameraWorldPos,
        FwdFogDensity, FwdFogHeightFalloff, FwdFogBaseHeight,
        FwdFogStartDistance, FwdFogCutoffDistance, FwdFogMaxOpacity);
    color.rgb = lerp(color.rgb, FwdFogColor.rgb, fogFactor);
    return color;
}

struct FMaterialPixelInput
{
    float2 UV0;
    float2 UV1;
    float2 UV2;
    float4 ParticleColor;
    float4 VertexColor;
    float  Time;
    float  SubImageIndex;
    float4 DynamicParam;
};

struct FMaterialResult
{
    float3 Color;
    float3 Emissive;
    float Opacity;
    float2 UVOffset;
};

Texture2D Tex_Diffuse : register(t0);

FMaterialResult EvaluateMaterial(FMaterialPixelInput Input)
{
    float2 n_3 = Input.UV0;
    float4 n_5 = Tex_Diffuse.Sample(LinearWrapSampler, n_3);
    float4 n_14 = Input.VertexColor;
    float3 n_21 = ((n_5).rgb * (n_14).rgb);
    float3 n_25 = (float4(n_21, 1.0f)).rgb;
    float n_79 = ((n_5).a * (n_14).a);
    FMaterialResult Result;
    Result.Color = n_25;
    Result.Emissive = float3(0, 0, 0);
    Result.Opacity = n_79;
    Result.UVOffset = float2(0, 0);
    return Result;
}


struct VS_Input_MaterialBeamTrailParticle
{
    float3 position : POSITION;
    float4 color    : COLOR;
    float2 texcoord : TEXTCOORD;
};

struct PS_Input_MaterialBeamTrailParticle
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
    float2 texcoord : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

PS_Input_MaterialBeamTrailParticle VS(VS_Input_MaterialBeamTrailParticle input)
{
    PS_Input_MaterialBeamTrailParticle output;
    output.position = ApplyVP(input.position);
    output.color    = input.color;
    output.texcoord = input.texcoord;
    output.worldPos = input.position;
    return output;
}

float4 PS(PS_Input_MaterialBeamTrailParticle input) : SV_TARGET
{
    FMaterialPixelInput MaterialInput;
    MaterialInput.UV0           = input.texcoord;
    MaterialInput.UV1           = float2(0, 0);
    MaterialInput.UV2           = float2(0, 0);
    MaterialInput.ParticleColor = input.color;
    MaterialInput.VertexColor   = input.color;
    MaterialInput.Time          = Time;
    MaterialInput.SubImageIndex = 0.0f;
    MaterialInput.DynamicParam  = float4(0, 0, 0, 0);

    FMaterialResult Result = EvaluateMaterial(MaterialInput);
    float4 FinalColor = float4(Result.Color + Result.Emissive, Result.Opacity);
    return ApplyFogTransparent(FinalColor, input.worldPos, CameraWorldPos);
}
