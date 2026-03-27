#include "Asset/StaticMesh/StaticMeshManager.h"

// 구현 메모:
// - LoadStaticMeshAsset()
//   1) NormalizeSourcePath()
//   2) StaticMeshAssetMap cache 확인
//   3) FStaticMeshBakeIO::MakeBakePath / IsBakeUpToDate 사용
//   4) bake load 실패 시 FObjImporter::ImportFromFile 호출
//   5) 성공한 asset을 cache에 보관
//
// - LoadStaticMeshObject()
//   1) StaticMeshObjectMap 또는 TObjectIterator<UStaticMesh>로 기존 object 탐색
//   2) 없으면 LoadStaticMeshAsset() 호출
//   3) 팀원이 만든 UStaticMesh wrapper 생성 및 asset 연결
//
// - Clear()
//   : asset/object cache 비우기 정책 구현
