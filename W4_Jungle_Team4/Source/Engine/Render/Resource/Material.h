#pragma once

#include "Object/Object.h"
#include "Texture.h"
#include "Core/CoreTypes.h"
#include "Math/Vector.h"

//cooked된 material 데이터
struct FMaterial
{
	FString MaterialName;

	//반사지수
	float Ns;

	//굴절률
	float Ni;

	//투명성, 1미만 일시 투명도 적용
	float d;

	//d의 inverse
	float Tr;

	//빛의 투과 속성
	float Tf;

	//재료에 사용할 조명 모델
	int32 illum;

	//기본 재질 색상(diffuse color)
	FVector Kd = {0.1f,0.1f,0.1f};

	//주변 반사율을 지정(Ambient)
	FVector Ka;
	
	//정반사율 지정 (specular color)
	FVector Ks;

	//표면에서 방출되는 빛의 색상 (emission color)
	FVector Ke;

	//diffuse Texture map
	FString map_Kd;
	
	//ambient Texture map
	FString map_Ka;

	//specular Texture map
	FString map_Ks;

	//specular highlight component
	FString map_Ns;
	
	//bump map (normal map)
	FString map_bump;
	
	//alpha Texture map
	FString map_d;

	
    bool HasTexture() const
    {
        return !map_Kd.empty()
            || !map_Ka.empty()
            || !map_Ks.empty();
    }
};

class UMaterial : public UObject
{
public:
	DECLARE_CLASS(UMaterial, UObject)

	UMaterial() {}
	~UMaterial() override;
	    
          FMaterial& GetMaterialData()       { return MaterialData; }
	const FMaterial& GetMaterialData() const { return MaterialData; }

    const FString& GetMaterialName() const   { return MaterialData.MaterialName; }

	// Ka / Kd / Ks / Ke
    FVector GetKa() const           { return MaterialData.Ka; }
    void    SetKa(FVector InKa)     { MaterialData.Ka = InKa; }
 
    FVector GetKd() const           { return MaterialData.Kd; }
    void    SetKd(FVector InKd)     { MaterialData.Kd = InKd; }
 
    FVector GetKs() const           { return MaterialData.Ks; }
    void    SetKs(FVector InKs)     { MaterialData.Ks = InKs; }
 
    FVector GetKe() const           { return MaterialData.Ke; }
    void    SetKe(FVector InKe)     { MaterialData.Ke = InKe; }
 
    float   GetNs() const           { return MaterialData.Ns; }
    void    SetNs(float InNs)       { MaterialData.Ns = InNs; }
 
    float   GetOpacity() const      { return MaterialData.d; }
    void    SetOpacity(float InD)   { MaterialData.d = InD; }
 
    // -------------------------------------------------------------------------
    // 텍스처 참조 (SRV는 UTexture 내부에 있다)
    // -------------------------------------------------------------------------
 
    // FObjManager가 MTL 파싱 후 UTexture를 찾아 연결해준다.
    void      SetDiffuseTexture(UTexture* InTex)   { DiffuseTexture  = InTex; }
    void      SetNormalTexture(UTexture* InTex)    { NormalTexture   = InTex; }
    void      SetSpecularTexture(UTexture* InTex)  { SpecularTexture = InTex; }
 
    UTexture* GetDiffuseTexture()  const           { return DiffuseTexture;  }
    UTexture* GetNormalTexture()   const           { return NormalTexture;   }
    UTexture* GetSpecularTexture() const           { return SpecularTexture; }
 
	void SetMaterialData(const FMaterial& InMaterialData) { MaterialData = InMaterialData; }
 
private:
    FMaterial  MaterialData;                // 셰이더 상수 버퍼용 CPU 데이터
 
    UTexture*  DiffuseTexture  = nullptr;   // map_Kd
    UTexture*  NormalTexture   = nullptr;   // map_bump
    UTexture*  SpecularTexture = nullptr;   // map_Ks
};

