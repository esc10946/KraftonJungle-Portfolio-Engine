/* Constant Buffers */


cbuffer FrameBuffer : register(b0)
{
    row_major float4x4 View;
    row_major float4x4 Projection;
    float bIsWireframe;
    float3 WireframeRGB;
}

cbuffer PerObjectBuffer : register(b1)
{
    row_major float4x4 Model;
    float4 PrimitiveColor; 
};

cbuffer GizmoBuffer : register(b2)
{
    float4 GizmoColorTint;
    uint bIsInnerGizmo;
    uint bClicking;
    uint SelectedAxis;
    float HoveredAxisOpacity;
};


cbuffer OverlayBuffer : register(b3)
{
    float2 OverlayCenterScreen;
    float2 ViewportSize;

    float OverlayRadius;
    float3 Padding2;

    float4 OverlayColor;
};

cbuffer EditorBuffer : register(b4)
{
    float4 CameraPosition;
    uint EditorFlag;
    float3 Padding3;
};

cbuffer OutlineConstants : register(b5)
{
    float4 OutlineColor;
    float3 OutlineInvScale;
    float OutlineOffset;
    uint PrimitiveType; //  0 : 2D, 1 : 3D
    float3 Padding4;
};

cbuffer StaticMeshBuffer : register(b6)
{
    uint StaticMeshUseTexture;
    uint StaticMeshUseAtlas;
    uint StaticMeshSourceUVsUseAtlasTiles;
    float StaticMeshSourceAtlasTileScale;
    float4 StaticMeshTopUVRect;
    float4 StaticMeshSideUVRect;
    float4 StaticMeshBottomUVRect;
};


float4 ApplyMVP(float3 pos)
{
    float4 world = mul(float4(pos, 1.0f), Model);
    float4 view = mul(world, View);
    return mul(view, Projection);
}

static const float PI = 3.14159265f;

float3 GetLightDirectionWS()
{
    // Surface -> directional light
    return normalize(float3(0.45f, 0.30f, 0.84f));
}

float3 GetLightColor()
{
    // Slightly warm sunlight
    return float3(1.00f, 0.96f, 0.90f) * 6.0f;
}

float3 GetSkyZenithColor()
{
    return float3(0.16f, 0.28f, 0.52f);
}

float3 GetSkyHorizonColor()
{
    return float3(0.72f, 0.78f, 0.88f);
}

float3 GetGroundColor()
{
    return float3(0.10f, 0.09f, 0.08f);
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - saturate(cosTheta), 5.0f);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = max(roughness * roughness, 0.045f);
    float a2 = a * a;
    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    return a2 / max(PI * denom * denom, 0.0001f);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return NdotV / max(NdotV * (1.0f - k) + k, 0.0001f);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

float3 SampleSkyEnvironment(float3 dir)
{
    float up = saturate(dir.z * 0.5f + 0.5f);
    float horizon = saturate(1.0f - abs(dir.z));
    float skyBlend = pow(up, 0.65f);
    float3 sky = lerp(GetSkyHorizonColor(), GetSkyZenithColor(), skyBlend);
    float3 env = lerp(GetGroundColor(), sky, up);
    env += horizon * float3(0.05f, 0.04f, 0.03f);

    float sunAmount = pow(saturate(dot(dir, GetLightDirectionWS())), 1024.0f);
    env += GetLightColor() * (sunAmount * 0.15f);
    return env;
}

float3 EvaluateDiffuseIBL(float3 baseColor, float metallic, float3 N)
{
    float3 skyIrradiance = SampleSkyEnvironment(N);
    return skyIrradiance * baseColor * (1.0f - metallic) * 0.28f;
}

float3 EvaluateSpecularIBL(float3 baseColor, float metallic, float roughness, float3 N, float3 V)
{
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), baseColor, metallic);
    float3 F = FresnelSchlick(saturate(dot(N, V)), F0);
    float3 R = reflect(-V, N);
    float3 env = SampleSkyEnvironment(R);
    float envLuma = dot(env, float3(0.299f, 0.587f, 0.114f));
    float blur = roughness * roughness;
    env = lerp(env, envLuma.xxx, blur * 0.85f);
    env *= lerp(1.15f, 0.35f, blur);
    return env * F;
}

float3 EvaluateDirectionalLightPBR(float3 baseColor, float metallic, float roughness, float3 N, float3 V)
{
    float3 L = GetLightDirectionWS();
    float3 H = normalize(V + L);
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    if (NdotL <= 0.0001f || NdotV <= 0.0001f)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), baseColor, metallic);
    float3 F = FresnelSchlick(saturate(dot(H, V)), F0);
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);

    float3 numerator = D * G * F;
    float denominator = max(4.0f * NdotV * NdotL, 0.001f);
    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = (1.0f.xxx - kS) * (1.0f - metallic);
    float3 diffuse = kD * baseColor / PI;
    return (diffuse + specular) * GetLightColor() * NdotL;
}
