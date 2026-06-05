#pragma once

#include "Object/Object.h"
#include "Object/Ptr/WeakObjectPtr.h"
#include "Core/Types/CollisionTypes.h"
#include "Navigation/NavTypes.h"

class AActor;
class ANavMeshBoundsVolume;
class ANavigationData;
class AGridNavMesh;
class UWorld;

#include "Source/Engine/Navigation/NavigationSystem.generated.h"

// ============================================================
// UNavigationSystem — 월드 단위 네비게이션 서비스.
// Recast가 없는 현재 엔진에서 언리얼식 API를 먼저 고정하고,
// 내부 구현은 floor projection + grid A* + physics sweep으로 제공한다.
// ============================================================
UCLASS()
class UNavigationSystem : public UObject
{
public:
	GENERATED_BODY()
	UNavigationSystem() = default;
	~UNavigationSystem() override = default;

	UFUNCTION(Pure, Category="Navigation")
	static UNavigationSystem* GetCurrent(UWorld* World);

	UFUNCTION(Callable, Category="Navigation")
	bool RebuildNavigation();
	UFUNCTION(Callable, Category="Navigation")
	void InvalidateNavigationData();

	UFUNCTION(Pure, Category="Navigation")
	ANavigationData* GetMainNavigationData() const;
	UFUNCTION(Pure, Category="Navigation")
	AGridNavMesh* GetGridNavMesh() const;
	UFUNCTION(Pure, Category="Navigation")
	bool HasBuiltNavigationData() const;
	UFUNCTION(Callable, Category="Navigation|Debug")
	void DebugDrawNavigationData(float Duration = 0.0f) const;
	UFUNCTION(Pure, Category="Navigation|Debug")
	FString GetLastQueryMessage() const { return LastQueryMessage; }
	UFUNCTION(Pure, Category="Navigation|Debug")
	FString GetDebugSummary() const;

	UFUNCTION(Callable, Category="Navigation")
	bool ProjectPointToNavigation(const FVector& Point, FVector& OutProjectedPoint, const FNavAgentProperties& AgentProps) const;

	UFUNCTION(Callable, Category="Navigation")
	bool FindPathSync(const FVector& Start, const FVector& End, const FNavAgentProperties& AgentProps, FNavigationPath& OutPath) const;

	UFUNCTION(Callable, Category="Navigation")
	bool NavigationRaycast(const FVector& Start, const FVector& End, const FNavAgentProperties& AgentProps) const;

	UFUNCTION(Callable, Category="Navigation")
	bool GetRandomReachablePointInRadius(const FVector& Origin, float Radius, FVector& OutPoint, const FNavAgentProperties& AgentProps) const;

	UFUNCTION(Pure, Category="Navigation")	
	bool HasNavigationBounds() const;
	UFUNCTION(Pure, Category="Navigation")
	bool IsPointInsideNavigationBounds(const FVector& Point) const;

	UPROPERTY(Edit, Save, Category="Navigation", DisplayName="Cell Size", Min=0.1f, Max=50.0f, Speed=0.1f)
	float CellSize = 2.0f;
	UPROPERTY(Edit, Save, Category="Navigation", DisplayName="Max Search Nodes", Min=16.0f, Max=65536.0f, Speed=64.0f)
	int32 MaxSearchNodes = 4096;
	UPROPERTY(Edit, Save, Category="Navigation", DisplayName="Projection Up", Min=0.0f, Max=100.0f, Speed=0.1f)
	float ProjectionUp = 5.0f;
	UPROPERTY(Edit, Save, Category="Navigation", DisplayName="Projection Down", Min=0.0f, Max=100.0f, Speed=0.1f)
	float ProjectionDown = 10.0f;
	UPROPERTY(Edit, Save, Category="Navigation", DisplayName="Direct Path Segment Length", Min=0.1f, Max=100.0f, Speed=0.1f)
	float DirectPathSegmentLength = 2.5f;
	UPROPERTY(Edit, Save, Category="Navigation", DisplayName="Use Built Navigation Data")
	bool bUseNavigationData = true;
	UPROPERTY(Edit, Save, Category="Navigation", DisplayName="Auto Rebuild On Path Request")
	bool bAutoRebuildOnPathRequest = true;
	UPROPERTY(Edit, Save, Category="Navigation", DisplayName="Allow Runtime Fallback")
	bool bAllowRuntimeFallback = true;

	// Raw runtime helpers used by NavigationData builders. These do not query cached
	// NavigationData again, so they are safe to call while building a nav graph.
	bool ProjectPointToNavigationRuntime(const FVector& Point, FVector& OutProjectedPoint, const FNavAgentProperties& AgentProps, float InProjectionUp = -1.0f, float InProjectionDown = -1.0f) const;
	bool CanTraverseSegmentRuntime(const FVector& Start, const FVector& End, const FNavAgentProperties& AgentProps) const;
	bool IsWalkableFloorHitRuntime(const FHitResult& Hit, const FNavAgentProperties& AgentProps) const;

private:
	UWorld* GetOwningWorld() const;
	bool IsPointAllowedByBounds(const FVector& Point) const;
	bool IsWalkableFloorHit(const FHitResult& Hit, const FNavAgentProperties& AgentProps) const;
	bool HasBlockingObstacleBetween(const FVector& Start, const FVector& End, const FNavAgentProperties& AgentProps) const;
	bool CanTraverseSegment(const FVector& Start, const FVector& End, const FNavAgentProperties& AgentProps) const;
	bool BuildDirectPathIfReachable(const FVector& Start, const FVector& End, const FNavAgentProperties& AgentProps, FNavigationPath& OutPath) const;
	bool FindGridPath(const FVector& Start, const FVector& End, const FNavAgentProperties& AgentProps, FNavigationPath& OutPath) const;
	void SmoothPath(const FNavAgentProperties& AgentProps, FNavigationPath& InOutPath) const;
	ANavigationData* FindNavigationDataActor() const;
	AGridNavMesh* EnsureGridNavMeshActor();
	bool SetLastQueryMessage(const FString& Message) const { LastQueryMessage = Message; return false; }

	mutable TWeakObjectPtr<ANavigationData> CachedMainNavData = nullptr;
	mutable FString LastQueryMessage;
};
