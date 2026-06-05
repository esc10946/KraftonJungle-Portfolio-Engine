#pragma once

#include "Core/Types/CoreTypes.h"
#include "Core/Types/EngineTypes.h"
#include "Math/Vector.h"

// ============================================================
// Debug draw categories.  Most callers use General.  Long-lived
// systems such as navigation use their own category so they can
// replace old debug lines instead of stacking multiple generations.
// ============================================================
enum class EDebugDrawCategory : uint8
{
	General = 0,
	Navigation = 1,
};

// ============================================================
// FDebugDrawItem — 단일 디버그 라인 (Box/Sphere는 라인으로 분해하여 저장)
// ============================================================
struct FDebugDrawItem
{
	FVector Start;
	FVector End;
	FColor  Color;
	float   RemainingTime;  // <= 0이면 이번 프레임 후 제거 (0 = 1프레임만)
	bool    bOneFrame;      // Duration == 0으로 추가된 항목 (1프레임 표시 후 제거)
	EDebugDrawCategory Category = EDebugDrawCategory::General;
};

// ============================================================
// FDebugDrawQueue — Duration 기반 디버그 라인 저장소
//
// DrawDebugHelpers에서 호출 → 라인으로 분해하여 축적
// 매 프레임 Tick()으로 Duration 감소 + 만료 항목 제거
// RenderCollector가 GetItems()로 읽어서 FScene에 제출
// ============================================================
class FDebugDrawQueue
{
public:
	void AddLine(const FVector& Start, const FVector& End,
		const FColor& Color, float Duration,
		EDebugDrawCategory Category = EDebugDrawCategory::General);

	void AddBox(const FVector& Center, const FVector& Extent,
		const FColor& Color, float Duration,
		EDebugDrawCategory Category = EDebugDrawCategory::General);

	void AddSphere(const FVector& Center, float Radius, int32 Segments,
		const FColor& Color, float Duration,
		EDebugDrawCategory Category = EDebugDrawCategory::General);

	// Duration 감소 + 만료 항목 제거. 매 프레임 Tick 시 호출.
	void Tick(float DeltaTime);

	const TArray<FDebugDrawItem>& GetItems() const { return Items; }

	void Clear() { Items.clear(); }
	void ClearCategory(EDebugDrawCategory Category);
	int32 GetItemCount(EDebugDrawCategory Category) const;

private:
	TArray<FDebugDrawItem> Items;
};
