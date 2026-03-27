#pragma once

#include "Object/Object.h"
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

	void SetMaterialData(const FMaterial& InMaterialData) { MaterialData = InMaterialData; }

	//GETSET함수 만들기
	
    FString   SourcePath;
    FMaterial MaterialData;
};

