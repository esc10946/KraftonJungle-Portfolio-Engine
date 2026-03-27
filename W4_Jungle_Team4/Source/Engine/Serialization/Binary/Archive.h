#pragma once

#include <cstddef>

// binary bake 입출력의 공통 인터페이스입니다.
// "어디에 어떻게 저장할지"는 writer/reader 구현체가 맡고,
// BakeIO는 이 인터페이스만 보고 데이터를 직렬화하도록 분리합니다.
class FArchive
{
public:
	virtual ~FArchive() = default;

	// 현재 archive가 로딩 모드인지 반환합니다.
	virtual bool IsLoading() const = 0;

	// 현재 archive가 저장 모드인지 반환합니다.
	virtual bool IsSaving() const = 0;

	// 스트림이 정상적으로 열려 있는지 반환합니다.
	virtual bool IsValid() const = 0;

	// 주어진 메모리 블록을 그대로 읽거나 씁니다.
	// Save 쪽에서는 "쓰기", Load 쪽에서는 "읽기"로 해석됩니다.
	virtual bool Serialize(void* InOutData, size_t InSize) = 0;

	// 파일/스트림을 명시적으로 닫습니다.
	virtual void Close() = 0;
};
