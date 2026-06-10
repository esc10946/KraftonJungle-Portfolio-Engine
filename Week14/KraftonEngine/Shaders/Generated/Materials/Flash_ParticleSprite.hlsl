// Generated from Content/Material/Particle/Parry/Flash.uasset
// Domain: ParticleSprite

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
    float4 n_21 = Tex_Diffuse.Sample(LinearWrapSampler, Input.UV0);
    float4 n_43 = Input.ParticleColor;
    float3 n_31 = ((n_21).rgb * (n_43).rgb);
    float3 n_53 = (float4(n_31, 1.0f)).rgb;
    FMaterialResult Result;
    Result.Color = n_53;
    Result.Emissive = float3(0, 0, 0);
    Result.Opacity = (n_21).a;
    Result.UVOffset = float2(0, 0);
    return Result;
}


struct VS_Input_MaterialSpriteParticle
{
    float3 cornerSign   : POSITION;
    float2 baseUV       : TEXTCOORD;
    float3 center       : INSTANCE_CENTER;
    float3 velocity     : INSTANCE_VELOCITY;
    float2 size         : INSTANCE_SIZE;
    float  rotation     : INSTANCE_ROTATION;
    float4 instColor    : INSTANCE_COLOR;
    int    subImage     : INSTANCE_SUBIMAGE;
    int    alignment    : INSTANCE_ALIGNMENT;
    float4 dynamicParam : INSTANCE_DYNAMICPARAM;
};

struct PS_Input_MaterialParticle
{
    float4 position       : SV_POSITION;
    float2 texcoord       : TEXCOORD0;
    float4 color          : COLOR;
    nointerpolation int subImageIndex : TEXCOORD1;
    float4 dynamicParam   : TEXCOORD2;
    float3 worldPos       : TEXCOORD3;
};

PS_Input_MaterialParticle VS(VS_Input_MaterialSpriteParticle input)
{
    float sinR = sin(input.rotation);
    float cosR = cos(input.rotation);

    float2 rotatedCorner = float2(
        input.cornerSign.x * cosR - input.cornerSign.y * sinR,
        input.cornerSign.x * sinR + input.cornerSign.y * cosR
    );

    const int ALIGN_SQUARE = 0;
    const int ALIGN_VELOCITY = 2;
    const int ALIGN_FACING = 3;
    float width = input.size.x;
    float height = (input.alignment == ALIGN_SQUARE) ? input.size.x : input.size.y;

    float4 clipPosition;
    float3 worldPos = input.center;
    if (input.alignment == ALIGN_FACING)
    {
        float3 cameraUp = float3(View._m01, View._m11, View._m21);
        float3 forward = normalize(CameraWorldPos - input.center);
        float3 right = normalize(cross(cameraUp, forward));
        float3 up = cross(forward, right);
        worldPos += (rotatedCorner.x * width) * right
                  + (rotatedCorner.y * height) * up;
        clipPosition = ApplyVP(worldPos);
    }
    else
    {
        float4 viewCenter = mul(float4(input.center, 1.0f), View);
        float2 axisRight = float2(1, 0);
        float2 axisUp = float2(0, 1);
        if (input.alignment == ALIGN_VELOCITY)
        {
            float2 viewVelocity = mul(float4(input.velocity, 0.0f), View).xy;
            if (dot(viewVelocity, viewVelocity) > 1e-8f)
            {
                axisUp = normalize(viewVelocity);
                axisRight = float2(axisUp.y, -axisUp.x);
            }
        }
        viewCenter.xy += (rotatedCorner.x * width) * axisRight
                       + (rotatedCorner.y * height) * axisUp;
        clipPosition = mul(viewCenter, Projection);
    }

    PS_Input_MaterialParticle output;
    output.position       = clipPosition;
    output.texcoord       = input.baseUV;
    output.color          = input.instColor;
    output.subImageIndex  = input.subImage;
    output.dynamicParam   = input.dynamicParam;
    output.worldPos       = worldPos;
    return output;
}

float4 PS(PS_Input_MaterialParticle input) : SV_TARGET
{
    FMaterialPixelInput MaterialInput;
    MaterialInput.UV0           = input.texcoord;
    MaterialInput.UV1           = float2(0, 0);
    MaterialInput.UV2           = float2(0, 0);
    MaterialInput.ParticleColor = input.color;
    MaterialInput.VertexColor   = input.color;
    MaterialInput.Time          = Time;
    MaterialInput.SubImageIndex = (float)input.subImageIndex;
    MaterialInput.DynamicParam  = input.dynamicParam;

    FMaterialResult Result = EvaluateMaterial(MaterialInput);
    float4 FinalColor = float4(Result.Color + Result.Emissive, Result.Opacity);
    clip(FinalColor.a - 0.01f);
    return ApplyFogTransparent(FinalColor, input.worldPos, CameraWorldPos);
}
