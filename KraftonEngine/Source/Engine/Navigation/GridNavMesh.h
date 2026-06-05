#pragma once

#include "Navigation/NavigationData.h"
#include "Core/Types/EngineTypes.h"

#include <unordered_map>
#include <unordered_set>

class ANavMeshBoundsVolume;
class UNavigationSystem;

#include "Source/Engine/Navigation/GridNavMesh.generated.h"

struct FGridNavCellKey
{
	int32 X = 0;
	int32 Y = 0;

	bool operator==(const FGridNavCellKey& Other) const
	{
		return X == Other.X && Y == Other.Y;
	}
};

struct FGridNavCellKeyHash
{
	size_t operator()(const FGridNavCellKey& Key) const
	{
		const uint64 A = static_cast<uint32>(Key.X);
		const uint64 B = static_cast<uint32>(Key.Y);
		return static_cast<size_t>((A << 32) ^ B ^ 0x9e3779b97f4a7c15ull);
	}
};

struct FGridNavCell
{
	FVector Location = FVector::ZeroVector;
	bool bWalkable = false;
};

// ============================================================
// AGridNavMesh
// Recast를 붙이기 전 단계의 실제 NavigationData 구현.
// NavMeshBoundsVolume 내부를 grid로 스캔해 walkable cell 그래프를 캐시한다.
// ============================================================
UCLASS()
class AGridNavMesh : public ANavigationData
{
public:
	GENERATED_BODY()
	AGridNavMesh();
	~AGridNavMesh() override = default;

	bool RebuildNavigationData() override;
	void ClearNavigationData() override;

	bool ProjectPointToNavigation(const FVector& Point, FVector& OutProjectedPoint, const FNavAgentProperties& AgentProps) const override;
	bool FindPath(const FVector& Start, const FVector& End, const FNavAgentProperties& AgentProps, FNavigationPath& OutPath) const override;
	bool NavigationRaycast(const FVector& Start, const FVector& End, const FNavAgentProperties& AgentProps) const override;
	bool GetRandomReachablePointInRadius(const FVector& Origin, float Radius, FVector& OutPoint, const FNavAgentProperties& AgentProps) const override;

	void DebugDrawNavigationData(float Duration = 0.0f) const override;

	UFUNCTION(Pure, Category="Navigation|Grid")
	int32 GetNavigationNodeCount() const override { return static_cast<int32>(WalkableCells.size()); }
	UFUNCTION(Pure, Category="Navigation|Grid")
	int32 GetBlockedNodeCount() const override { return BlockedCellCount; }
	UFUNCTION(Pure, Category="Navigation|Grid")
	float GetCellSize() const { return CellSize; }
	UFUNCTION(Pure, Category="Navigation|Grid")
	int32 GetLastBuildBoundsCount() const { return LastBuildBoundsCount; }
	UFUNCTION(Pure, Category="Navigation|Grid")
	int32 GetLastBuildCandidateCount() const { return LastBuildCandidateCount; }
	UFUNCTION(Pure, Category="Navigation|Grid")
	float GetLastBuildDurationMs() const { return LastBuildDurationMs; }
	UFUNCTION(Pure, Category="Navigation|Grid")
	FString GetLastBuildMessage() const { return LastBuildMessage; }
	UFUNCTION(Callable, Category="Navigation|Grid")
	void SetCellSize(float InCellSize);
	UFUNCTION(Pure, Category="Navigation|Grid")
	bool IsLocationOnNavData(const FVector& Location) const;

	UPROPERTY(Edit, Save, Category="Navigation|Grid", DisplayName="Cell Size", Min=0.1f, Max=50.0f, Speed=0.1f)
	float CellSize = 2.0f;
	UPROPERTY(Edit, Save, Category="Navigation|Grid", DisplayName="Max Search Nodes", Min=16.0f, Max=65536.0f, Speed=64.0f)
	int32 MaxSearchNodes = 8192;
	UPROPERTY(Edit, Save, Category="Navigation|Grid", DisplayName="Projection Up", Min=0.0f, Max=100.0f, Speed=0.1f)
	float ProjectionUp = 5.0f;
	UPROPERTY(Edit, Save, Category="Navigation|Grid", DisplayName="Projection Down", Min=0.0f, Max=100.0f, Speed=0.1f)
	float ProjectionDown = 10.0f;
	UPROPERTY(Edit, Save, Category="Navigation|Grid", DisplayName="Direct Path Segment Length", Min=0.1f, Max=100.0f, Speed=0.1f)
	float DirectPathSegmentLength = 2.5f;
	UPROPERTY(Edit, Save, Category="Navigation|Grid", DisplayName="Walkable Floor Max Thickness", Min=0.01f, Max=100.0f, Speed=0.1f)
	float WalkableFloorMaxThickness = 1.0f;
	UPROPERTY(Edit, Save, Category="Navigation|Grid", DisplayName="Nearest Cell Search Rings", Min=1.0f, Max=64.0f, Speed=1.0f)
	int32 NearestCellSearchRings = 12;
	UPROPERTY(Edit, Save, Category="Navigation|Debug", DisplayName="Draw On Build")
	bool bDrawDebugOnBuild = false;
	UPROPERTY(Edit, Save, Category="Navigation|Debug", DisplayName="Debug Draw Duration", Min=0.0f, Max=60.0f, Speed=0.1f)
	float DebugDrawDuration = 0.0f;

private:
	struct FBuildBounds
	{
		FBoundingBox Bounds;
	};

	UNavigationSystem* GetNavigationSystem() const;
	bool CollectBuildBounds(TArray<FBuildBounds>& OutBounds) const;
	FGridNavCellKey WorldToCell(const FVector& Point) const;
	FVector CellToWorldGuess(const FGridNavCellKey& Key, float Z) const;
	bool IsInsideAnyBuildBounds(const FVector& Point, const TArray<FBuildBounds>& Bounds) const;
	bool BuildCell(const FGridNavCellKey& Key, float GuessZ, const TArray<FBuildBounds>& Bounds, FGridNavCell& OutCell) const;
	bool ProjectPointToWalkableSurfaceByPrimitiveBounds(const FVector& Guess, const TArray<FBuildBounds>& Bounds, FVector& OutProjectedLocation) const;
	bool HasAgentFootprint(const FVector& ProjectedLocation) const;
	bool HasAgentFootprintByPrimitiveBounds(const FVector& ProjectedLocation) const;
	const FGridNavCell* FindCell(const FGridNavCellKey& Key) const;
	bool FindNearestCell(const FVector& Point, FGridNavCellKey& OutKey, FVector& OutLocation) const;
	bool HasCachedSegment(const FVector& Start, const FVector& End, const FNavAgentProperties& AgentProps) const;
	bool BuildDirectCachedPath(const FVector& Start, const FVector& End, const FNavAgentProperties& AgentProps, FNavigationPath& OutPath) const;
	bool IsDiagonalMoveAllowed(const FGridNavCellKey& From, const FGridNavCellKey& Dir) const;
	void SmoothPath(const FNavAgentProperties& AgentProps, FNavigationPath& InOutPath) const;

	std::unordered_map<FGridNavCellKey, FGridNavCell, FGridNavCellKeyHash> WalkableCells;
	int32 BlockedCellCount = 0;
	int32 LastBuildBoundsCount = 0;
	int32 LastBuildCandidateCount = 0;
	float LastBuildDurationMs = 0.0f;
	FString LastBuildMessage;
	FBoundingBox CachedBounds;
};
