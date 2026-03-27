#include "Serialization/Binary/WindowsBinWriter.h"

// 구현 메모:
// - 생성자에서 std::ios::binary | std::ios::out 으로 열기
// - Serialize()는 write 호출
// - IsSaving()은 true, IsLoading()은 false
