#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct FTypeInfo;

// 에디터에서 자동 렌더링할 수 있는 최소 프로퍼티 타입 목록입니다.
enum class EPropertyType : uint8_t
{
	Bool,
	Int,
	Float,
	Vec3,
	Vec4,
	String,
	Object,
	Name,
};

// 컴포넌트가 에디터에 노출하는 편집 가능 프로퍼티 메타데이터입니다.
struct FPropertyDescriptor
{
	const char* Name = nullptr;
	EPropertyType Type = EPropertyType::String;
	void* ValuePtr = nullptr;

	float Min = 0.0f;
	float Max = 0.0f;
	float Speed = 0.1f;

	// Object picker가 보여줄 타입 필터입니다.
	const FTypeInfo* ObjectClass = nullptr;
};
