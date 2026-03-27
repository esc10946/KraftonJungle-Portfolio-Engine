#include "Asset/StaticMesh/StaticMeshBakeIO.h"

// 구현 메모:
// - MakeBakePath()      : 확장자 변경 및 bake 디렉터리 매핑
// - IsBakeUpToDate()    : filesystem timestamp 비교
// - SaveToFile()        : writer 생성 후 Save() 호출
// - LoadFromFile()      : reader 생성 후 Load() 호출
// - Save()/Load()       : version, path, vertices, indices, sections, bounds 순으로 직렬화
