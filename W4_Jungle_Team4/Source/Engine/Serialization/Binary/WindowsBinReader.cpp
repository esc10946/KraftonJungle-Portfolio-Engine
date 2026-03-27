#include "Serialization/Binary/WindowsBinReader.h"

// 구현 메모:
// - 생성자에서 std::ios::binary | std::ios::in 으로 열기
// - Serialize()는 read 호출
// - IsLoading()은 true, IsSaving()은 false
