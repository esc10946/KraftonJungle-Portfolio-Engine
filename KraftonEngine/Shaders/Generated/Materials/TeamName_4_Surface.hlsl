// Generated from C:/Users/jungle/GitHub/Jungle_Week14_Team6/KraftonEngine/Content/Material/Custom/TeamName_4.uasset
// Domain: Surface

#include "Common/ConstantBuffers.hlsli"
#include "Common/VertexLayouts.hlsli"
#include "Common/Functions.hlsli"
#include "Common/SystemSamplers.hlsli"
#include "Common/Skinning.hlsli"
#include "Common/ForwardLighting.hlsli"

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
    float3 BaseColor;
    float3 Normal;
    float Roughness;
    float Metallic;
    float3 Emissive;
    float Opacity;
};

Texture2D Tex_Diffuse : register(t0);

FMaterialResult EvaluateMaterial(FMaterialPixelInput Input)
{
    float4 n_17 = Tex_Diffuse.Sample(LinearWrapSampler, Input.UV0);
    float n_3 = 1.000000f;
    FMaterialResult Result;
    Result.BaseColor = (n_17).rgb;
    Result.Normal = float3(0, 0, 1);
    Result.Roughness = 0.5f;
    Result.Metallic = 0.0f;
    Result.Emissive = float3(0, 0, 0);
    Result.Opacity = n_3;
    return Result;
}


struct MaterialSurfaceVSOutput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

MaterialSurfaceVSOutput BuildMaterialSurfaceVS(float3 position, float3 normal, float4 color, float2 texcoord)
{
    MaterialSurfaceVSOutput output;
    float4 worldPos = mul(float4(position, 1.0f), Model);
    output.worldPos = worldPos.xyz;
    output.position = mul(mul(worldPos, View), Projection);
    output.normal = normalize(mul(normal, (float3x3)NormalMatrix));
    output.color = color;
    output.texcoord = texcoord;
    return output;
}

MaterialSurfaceVSOutput VS_StaticMesh(VS_Input_PNCTT input)
{
    return BuildMaterialSurfaceVS(input.position, input.normal, input.color, input.texcoord);
}

MaterialSurfaceVSOutput VS(VS_Input_PNCTT input)
{
    return VS_StaticMesh(input);
}

MaterialSurfaceVSOutput VS_SkeletalMesh(VS_Input_PNCTTBB input)
{
    FSkinningResult skinned = ApplyLinearBlendSkinning(
        input.position,
        input.normal,
        input.tangent.xyz,
        input.boneIndices,
        input.boneWeights);

    return BuildMaterialSurfaceVS(skinned.position.xyz, skinned.normal, input.color, input.texcoord);
}


float4 PS(MaterialSurfaceVSOutput input) : SV_TARGET
{

    FMaterialPixelInput MaterialInput;
    MaterialInput.UV0           = input.texcoord;
    MaterialInput.UV1           = float2(0, 0);
    MaterialInput.UV2           = float2(0, 0);
    MaterialInput.ParticleColor = float4(1, 1, 1, 1);
    MaterialInput.VertexColor   = input.color;
    MaterialInput.Time          = Time;
    MaterialInput.SubImageIndex = 0.0f;
    MaterialInput.DynamicParam  = float4(0, 0, 0, 0);

    FMaterialResult Result = EvaluateMaterial(MaterialInput);
    float3 N = normalize(input.normal);

    float3 V = normalize(CameraWorldPos - input.worldPos);
    float3 diffuse = AccumulateDiffuse(input.worldPos, N, input.position);
    float3 specular = AccumulateSpecular(input.worldPos, N, V, 32.0f, input.position);

    float3 finalRgb = Result.BaseColor * diffuse + specular + Result.Emissive;
    float OutOpacity = saturate(Result.Opacity);

    return float4(finalRgb, OutOpacity);
}
