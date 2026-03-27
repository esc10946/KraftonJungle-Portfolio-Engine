#pragma once

#include "Core/CoreTypes.h"
#include "Core/Singleton.h"

class UStaticMesh;
struct FStaticMeshAsset;

// 엔진 내부에서 StaticMesh asset 전체를 관리하는 중심 manager입니다.
// 이 클래스가 cache / bake / import 정책을 들고 있고,
// OBJ 전용 외부 API(FObjManager)는 이 manager를 감싸는 얇은 facade 역할만 하도록 설계합니다.
class FStaticMeshManager : public TSingleton<FStaticMeshManager>
{
	friend class TSingleton<FStaticMeshManager>;

public:
	// 경로에 대응하는 cooked asset을 반환합니다.
	// 호출 흐름은 보통 다음 순서를 기대합니다:
	// 1) 메모리 cache 확인
	// 2) bake 파일 존재/최신 여부 확인
	// 3) bake load 또는 FObjImporter import 실행
	// 4) 결과 asset cache 보관 후 반환
	FStaticMeshAsset* LoadStaticMeshAsset(const FString& InSourcePath);

	// 경로에 대응하는 UObject wrapper(UStaticMesh)를 반환합니다.
	// 다른 팀원이 UStaticMesh를 구현하면, 이 함수가 "이미 로드된 object 재사용"과
	// "없으면 asset 기반으로 새 object 생성"을 담당하는 진입점이 됩니다.
	UStaticMesh* LoadStaticMeshObject(const FString& InSourcePath);

	// 이미 메모리에 올라온 cooked asset만 조회합니다.
	// 실제 로드를 발생시키지 않기 때문에 디버그/에디터 UI에서 cache 상태 확인용으로 적합합니다.
	FStaticMeshAsset* FindLoadedStaticMeshAsset(const FString& InSourcePath);

	// 이미 생성된 UStaticMesh wrapper만 조회합니다.
	// UObject iterator 또는 object cache를 이용한 재사용 정책 구현 지점입니다.
	UStaticMesh* FindLoadedStaticMeshObject(const FString& InSourcePath);

	// 여러 OBJ 경로를 미리 준비합니다.
	// 에디터 초기 자산 preload, 데모 메시 사전 준비 등에 사용 가능합니다.
	void Preload(const TArray<FString>& InSourcePaths);

	// manager가 소유한 cache를 비웁니다.
	// 실제 해제 방식은 UObject 수명, bake 재사용 정책과 함께 정해야 하므로
	// 지금은 인터페이스만 정의해 둡니다.
	void Clear();

private:
	FStaticMeshManager() = default;

	// 캐시 키로 쓰기 전에 경로 문자열을 정규화합니다.
	// 상대/절대 경로, 슬래시 표기 차이로 같은 파일이 중복 로드되는 일을 줄이는 역할입니다.
	FString NormalizeSourcePath(const FString& InSourcePath) const;

private:
	// cooked asset cache입니다.
	// manager가 import/bake 결과를 재사용하기 위해 소유합니다.
	TMap<FString, FStaticMeshAsset*> StaticMeshAssetMap;

	// UObject wrapper cache입니다.
	// 팀원이 UStaticMesh를 구현하면 asset cache와 별개로 object 재사용 지점이 됩니다.
	TMap<FString, UStaticMesh*> StaticMeshObjectMap;
};
