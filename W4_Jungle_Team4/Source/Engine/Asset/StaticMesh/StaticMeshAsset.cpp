#include "Asset/StaticMesh/StaticMeshAsset.h"

// 구현 메모:
// - Reset()         : 모든 배열/경로/bounds 초기화
// - IsValid()       : vertex/index 유효성 검사
// - BuildRenderMeshData()
//                   : FStaticMeshVertex -> FMeshData/FVertex 변환
// - UpdateBounds()  : vertex min/max 누적 계산
