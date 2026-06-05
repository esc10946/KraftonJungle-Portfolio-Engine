#include "Navigation/GridNavMesh.h"

#include "Component/PrimitiveComponent.h"
#include "Component/Shape/BoxComponent.h"
#include "Debug/DrawDebugHelpers.h"
#include "Debug/DebugDrawQueue.h"
#include "GameFramework/Actor/NavMeshBoundsVolume.h"
#include "GameFramework/World.h"
#include "Math/MathUtils.h"
#include "Math/Quat.h"
#include "Navigation/NavigationSystem.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <string>
#include <cmath>
#include <cfloat>
#include <cstdlib>
#include <limits>
#include <queue>
#include <vector>

namespace
{
	struct FGridOpenNode
	{
		FGridNavCellKey Key;
		float F = 0.0f;
		float G = 0.0f;
		bool operator<(const FGridOpenNode& Other) const
		{
			return F > Other.F;
		}
	};

	struct FGridSearchRecord
	{
		FGridNavCellKey Parent;
		float G = FLT_MAX;
		bool bHasParent = false;
	};

	float KeyHeuristic(const FGridNavCellKey& A, const FGridNavCellKey& B)
	{
		const float DX = static_cast<float>(A.X - B.X);
		const float DY = static_cast<float>(A.Y - B.Y);
		return sqrtf(DX * DX + DY * DY);
	}

	float HorizontalDistanceSquared(const FVector& A, const FVector& B)
	{
		const float DX = A.X - B.X;
		const float DY = A.Y - B.Y;
		return DX * DX + DY * DY;
	}

	void AddPathPointIfDistinct(FNavigationPath& Path, const FVector& Location)
	{
		if (!Path.PathPoints.empty())
		{
			const FVector Last = Path.PathPoints.back().Location;
			if (FVector::DistSquared(Last, Location) < 0.01f)
			{
				return;
			}
		}
		Path.PathPoints.emplace_back(Location);
	}

	bool IsGridNavigationRelevantPrimitive(const UPrimitiveComponent* Primitive)
	{
		if (!Primitive)
		{
			return false;
		}
		if (Cast<ANavMeshBoundsVolume>(Primitive->GetOwner()))
		{
			return false;
		}
		return Primitive->CanEverAffectNavigation()
			&& Primitive->IsQueryCollisionEnabled()
			&& Primitive->GetCollisionObjectType() == ECollisionChannel::WorldStatic;
	}

	int32 ComputeDebugDrawStride(int32 TotalCells, int32 MaxCells)
	{
		if (MaxCells <= 0 || TotalCells <= MaxCells)
		{
			return 1;
		}
		return std::max(1, static_cast<int32>(ceilf(static_cast<float>(TotalCells) / static_cast<float>(MaxCells))));
	}

	float Clamp01(float Value)
	{
		return std::max(0.0f, std::min(1.0f, Value));
	}

	uint32 LerpByte(uint32 A, uint32 B, float Alpha)
	{
		const float T = Clamp01(Alpha);
		return static_cast<uint32>(static_cast<float>(A) + (static_cast<float>(B) - static_cast<float>(A)) * T);
	}

	FColor LerpColor(const FColor& A, const FColor& B, float Alpha)
	{
		return FColor(
			LerpByte(A.R, B.R, Alpha),
			LerpByte(A.G, B.G, Alpha),
			LerpByte(A.B, B.B, Alpha),
			LerpByte(A.A, B.A, Alpha));
	}
}


AGridNavMesh::AGridNavMesh()
{
	bNeedsTick = false;
	SupportedAgent.Radius = 0.42f;
	SupportedAgent.Height = 2.10f;
	SupportedAgent.StepHeight = 0.60f;
	SupportedAgent.MaxClimbHeight = 0.60f;
	SupportedAgent.MaxDropHeight = 0.60f;
	SupportedAgent.MaxSlopeDegrees = 50.0f;
}

UNavigationSystem* AGridNavMesh::GetNavigationSystem() const
{
	UWorld* World = GetWorld();
	return World ? World->GetNavigationSystem() : nullptr;
}

void AGridNavMesh::SetCellSize(float InCellSize)
{
	CellSize = std::max(0.25f, InCellSize);
	bNavigationDataBuilt = false;
}

void AGridNavMesh::SetSupportedAgentForBuild(const FNavAgentProperties& InAgent)
{
	SupportedAgent.Radius = std::max(0.01f, InAgent.Radius);
	SupportedAgent.Height = std::max(SupportedAgent.Radius * 2.0f, InAgent.Height);
	SupportedAgent.StepHeight = std::max(0.0f, InAgent.StepHeight);
	SupportedAgent.MaxClimbHeight = std::max(0.0f, InAgent.GetEffectiveMaxClimbHeight());
	SupportedAgent.MaxDropHeight = std::max(0.0f, InAgent.GetEffectiveMaxDropHeight());
	SupportedAgent.MaxSlopeDegrees = std::max(0.0f, std::min(89.0f, InAgent.MaxSlopeDegrees));
	bNavigationDataBuilt = false;
}

void AGridNavMesh::PostEditProperty(const char* PropertyName)
{
	Super::PostEditProperty(PropertyName);

	if (!PropertyName)
	{
		return;
	}

	const bool bNavigationBuildProperty =
		strcmp(PropertyName, "CellSize") == 0 || strcmp(PropertyName, "Cell Size") == 0 ||
		strcmp(PropertyName, "MaxSearchNodes") == 0 || strcmp(PropertyName, "Max Search Nodes") == 0 ||
		strcmp(PropertyName, "ProjectionUp") == 0 || strcmp(PropertyName, "Projection Up") == 0 ||
		strcmp(PropertyName, "ProjectionDown") == 0 || strcmp(PropertyName, "Projection Down") == 0 ||
		strcmp(PropertyName, "DirectPathSegmentLength") == 0 || strcmp(PropertyName, "Direct Path Segment Length") == 0 ||
		strcmp(PropertyName, "WalkableFloorMaxThickness") == 0 || strcmp(PropertyName, "Walkable Floor Max Thickness") == 0 ||
		strcmp(PropertyName, "ObstaclePadding") == 0 || strcmp(PropertyName, "Obstacle Padding") == 0 ||
		strcmp(PropertyName, "bUsePhysicsProjectionFallback") == 0 || strcmp(PropertyName, "Physics Projection Fallback") == 0 ||
		strcmp(PropertyName, "NearestCellSearchRings") == 0 || strcmp(PropertyName, "Nearest Cell Search Rings") == 0 ||
		strcmp(PropertyName, "bDrawDebugOnBuild") == 0 || strcmp(PropertyName, "Draw On Build") == 0 ||
		strcmp(PropertyName, "bDrawBlockedCells") == 0 || strcmp(PropertyName, "Draw Blocked Cells") == 0 ||
		strcmp(PropertyName, "bDrawHeightColors") == 0 || strcmp(PropertyName, "Draw Height Colors") == 0 ||
		strcmp(PropertyName, "bDrawHeightContours") == 0 || strcmp(PropertyName, "Draw Height Contours") == 0 ||
		strcmp(PropertyName, "DebugHeightContourInterval") == 0 || strcmp(PropertyName, "Height Contour Interval") == 0 ||
		strcmp(PropertyName, "DebugDrawDuration") == 0 || strcmp(PropertyName, "Debug Draw Duration") == 0 ||
		strcmp(PropertyName, "DebugDrawMaxCells") == 0 || strcmp(PropertyName, "Debug Draw Max Cells") == 0;

	if (!bNavigationBuildProperty)
	{
		return;
	}

	SetCellSize(CellSize);
	MaxSearchNodes = std::max(16, MaxSearchNodes);
	ProjectionUp = std::max(0.0f, ProjectionUp);
	ProjectionDown = std::max(0.0f, ProjectionDown);
	DirectPathSegmentLength = std::max(0.1f, DirectPathSegmentLength);
	WalkableFloorMaxThickness = std::max(0.01f, WalkableFloorMaxThickness);
	ObstaclePadding = std::max(0.0f, ObstaclePadding);
	NearestCellSearchRings = std::max(1, NearestCellSearchRings);
	DebugHeightContourInterval = std::max(0.0f, DebugHeightContourInterval);
	DebugDrawDuration = std::max(0.0f, DebugDrawDuration);
	DebugDrawMaxCells = std::max(0, DebugDrawMaxCells);

	if (UNavigationSystem* NavSys = GetNavigationSystem())
	{
		NavSys->RequestNavigationRebuild("GridNavMesh property edited", false);
	}
	else
	{
		RebuildNavigationData();
	}
}

void AGridNavMesh::ClearNavigationData()
{
	if (UWorld* World = GetWorld())
	{
		World->GetScene().GetDebugDrawQueue().ClearCategory(EDebugDrawCategory::Navigation);
	}
	WalkableCells.clear();
	BlockedCells.clear();
	BlockedCellCount = 0;
	LastBuildBoundsCount = 0;
	LastBuildCandidateCount = 0;
	LastBuildDurationMs = 0.0f;
	LastBuildMessage.clear();
	CachedBounds = FBoundingBox();
	bNavigationDataBuilt = false;
}

bool AGridNavMesh::CollectBuildBounds(TArray<FBuildBounds>& OutBounds) const
{
	OutBounds.clear();
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	for (AActor* Actor : World->GetActors())
	{
		ANavMeshBoundsVolume* Volume = Cast<ANavMeshBoundsVolume>(Actor);
		if (!Volume)
		{
			continue;
		}
		UBoxComponent* Box = Volume->GetBoundsComponent();
		if (!Box)
		{
			continue;
		}
		const FVector Center = Box->GetWorldLocation();
		const FVector Extent = Box->GetScaledBoxExtent();
		FBuildBounds Build;
		Build.Bounds = FBoundingBox(Center - Extent, Center + Extent);
		if (Build.Bounds.IsValid())
		{
			OutBounds.push_back(Build);
		}
	}

	if (!OutBounds.empty())
	{
		return true;
	}

	// Fallback: NavMeshBoundsVolume이 없으면 WorldStatic query collision primitive들의 AABB를 합쳐
	// 최소한의 NavigationData를 만들 수 있게 한다. 실제 게임 맵에서는 BoundsVolume 배치를 권장한다.
	FBoundingBox Combined;
	bool bHasStaticPrimitive = false;
	for (AActor* Actor : World->GetActors())
	{
		if (!Actor || Cast<ANavMeshBoundsVolume>(Actor))
		{
			continue;
		}
		for (UPrimitiveComponent* Primitive : Actor->GetPrimitiveComponents())
		{
			if (!IsGridNavigationRelevantPrimitive(Primitive))
			{
				continue;
			}
			const FBoundingBox Bounds = Primitive->GetWorldBoundingBox();
			if (!Bounds.IsValid())
			{
				continue;
			}
			Combined.Expand(Bounds.Min);
			Combined.Expand(Bounds.Max);
			bHasStaticPrimitive = true;
		}
	}

	if (!bHasStaticPrimitive || !Combined.IsValid())
	{
		return false;
	}

	const FVector Margin(CellSize * 2.0f, CellSize * 2.0f, ProjectionUp + ProjectionDown);
	FBuildBounds Build;
	Build.Bounds = FBoundingBox(Combined.Min - Margin, Combined.Max + Margin);
	OutBounds.push_back(Build);
	return true;
}

void AGridNavMesh::CollectNavigationPrimitives(TArray<FNavigationPrimitiveBounds>& OutPrimitives) const
{
	OutPrimitives.clear();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float MaxFloorThickness = std::max(0.01f, WalkableFloorMaxThickness);
	for (AActor* Actor : World->GetActors())
	{
		if (!Actor || Cast<ANavMeshBoundsVolume>(Actor))
		{
			continue;
		}
		for (UPrimitiveComponent* Primitive : Actor->GetPrimitiveComponents())
		{
			if (!IsGridNavigationRelevantPrimitive(Primitive))
			{
				continue;
			}
			const FBoundingBox Bounds = Primitive->GetWorldBoundingBox();
			if (!Bounds.IsValid())
			{
				continue;
			}
			const FVector Extent = Bounds.GetExtent();
			FNavigationPrimitiveBounds Entry;
			Entry.Bounds = Bounds;
			Entry.bFloorCandidate = Extent.Z <= MaxFloorThickness;
			Entry.bObstacle = !Entry.bFloorCandidate;
			OutPrimitives.push_back(Entry);
		}
	}
}

FGridNavCellKey AGridNavMesh::WorldToCell(const FVector& Point) const
{
	const float Grid = std::max(0.25f, CellSize);
	return FGridNavCellKey{
		static_cast<int32>(floorf(Point.X / Grid)),
		static_cast<int32>(floorf(Point.Y / Grid))
	};
}

FVector AGridNavMesh::CellToWorldGuess(const FGridNavCellKey& Key, float Z) const
{
	const float Grid = std::max(0.25f, CellSize);
	return FVector((static_cast<float>(Key.X) + 0.5f) * Grid, (static_cast<float>(Key.Y) + 0.5f) * Grid, Z);
}

bool AGridNavMesh::IsInsideAnyBuildBounds(const FVector& Point, const TArray<FBuildBounds>& Bounds) const
{
	if (Bounds.empty())
	{
		return true;
	}
	for (const FBuildBounds& B : Bounds)
	{
		if (B.Bounds.IsContains(Point))
		{
			return true;
		}
	}
	return false;
}

bool AGridNavMesh::ProjectPointToWalkableSurfaceByPrimitiveBounds(
	const FVector& Guess,
	const TArray<FBuildBounds>& Bounds,
	const TArray<FNavigationPrimitiveBounds>& Primitives,
	FVector& OutProjectedLocation) const
{
	if (!IsInsideAnyBuildBounds(Guess, Bounds))
	{
		return false;
	}

	const float HalfCell = std::max(0.05f, CellSize * 0.5f);
	const float StartZ = Guess.Z + ProjectionUp;
	const float EndZ = Guess.Z - ProjectionDown;
	bool bFound = false;
	float BestZ = -FLT_MAX;

	for (const FNavigationPrimitiveBounds& PrimitiveBounds : Primitives)
	{
		if (!PrimitiveBounds.bFloorCandidate)
		{
			continue;
		}
		const FBoundingBox& B = PrimitiveBounds.Bounds;
		if (!B.IsValid())
		{
			continue;
		}
		if (Guess.X < B.Min.X - HalfCell || Guess.X > B.Max.X + HalfCell ||
			Guess.Y < B.Min.Y - HalfCell || Guess.Y > B.Max.Y + HalfCell)
		{
			continue;
		}
		const float TopZ = B.Max.Z;
		if (TopZ < EndZ || TopZ > StartZ)
		{
			continue;
		}
		if (TopZ > BestZ)
		{
			BestZ = TopZ;
			bFound = true;
		}
	}

	if (!bFound)
	{
		return false;
	}
	OutProjectedLocation = FVector(Guess.X, Guess.Y, BestZ);
	return IsInsideAnyBuildBounds(OutProjectedLocation, Bounds);
}

bool AGridNavMesh::HasAgentFootprintByPrimitiveBounds(const FVector& ProjectedLocation, const TArray<FNavigationPrimitiveBounds>& Primitives) const
{
	const float Radius = std::max(0.05f, SupportedAgent.Radius + std::max(0.0f, ObstaclePadding));
	const float AgentBottom = ProjectedLocation.Z + 0.05f;
	const float AgentTop = ProjectedLocation.Z + std::max(Radius * 2.0f, SupportedAgent.Height);

	for (const FNavigationPrimitiveBounds& PrimitiveBounds : Primitives)
	{
		if (!PrimitiveBounds.bObstacle)
		{
			continue;
		}
		const FBoundingBox& B = PrimitiveBounds.Bounds;
		if (!B.IsValid())
		{
			continue;
		}
		if (B.Max.Z <= ProjectedLocation.Z + 0.02f)
		{
			continue;
		}
		if (B.Min.Z >= AgentTop || B.Max.Z <= AgentBottom)
		{
			continue;
		}
		if (ProjectedLocation.X >= B.Min.X - Radius && ProjectedLocation.X <= B.Max.X + Radius &&
			ProjectedLocation.Y >= B.Min.Y - Radius && ProjectedLocation.Y <= B.Max.Y + Radius)
		{
			return false;
		}
	}
	return true;
}

bool AGridNavMesh::HasAgentFootprint(const FVector& ProjectedLocation, const TArray<FNavigationPrimitiveBounds>& Primitives) const
{
	return HasAgentFootprintByPrimitiveBounds(ProjectedLocation, Primitives);
}

AGridNavMesh::FBuildCellResult AGridNavMesh::BuildCell(
	const FGridNavCellKey& Key,
	float GuessZ,
	const TArray<FBuildBounds>& Bounds,
	const TArray<FNavigationPrimitiveBounds>& Primitives,
	FGridNavCell& OutCell) const
{
	FBuildCellResult Result;
	const FVector Guess = CellToWorldGuess(Key, GuessZ);
	if (!IsInsideAnyBuildBounds(Guess, Bounds))
	{
		return Result;
	}
	FVector Projected;
	bool bProjected = ProjectPointToWalkableSurfaceByPrimitiveBounds(Guess, Bounds, Primitives, Projected);
	if (!bProjected && bUsePhysicsProjectionFallback)
	{
		if (UNavigationSystem* NavSys = GetNavigationSystem())
		{
			bProjected = NavSys->ProjectPointToNavigationRuntime(Guess, Projected, SupportedAgent, ProjectionUp, ProjectionDown);
		}
	}
	if (!bProjected || !IsInsideAnyBuildBounds(Projected, Bounds))
	{
		return Result;
	}
	Result.DebugLocation = Projected;
	if (!HasAgentFootprint(Projected, Primitives))
	{
		Result.bBlockedByObstacle = true;
		return Result;
	}
	OutCell.Location = Projected;
	OutCell.bWalkable = true;
	Result.bWalkable = true;
	return Result;
}

bool AGridNavMesh::RebuildNavigationData()
{
	const auto BuildStart = std::chrono::steady_clock::now();
	ClearNavigationData();

	TArray<FBuildBounds> Bounds;
	if (!CollectBuildBounds(Bounds))
	{
		LastBuildMessage = "GridNavMesh build failed: no NavMeshBoundsVolume or WorldStatic primitives";
		return false;
	}
	LastBuildBoundsCount = static_cast<int32>(Bounds.size());

	for (const FBuildBounds& B : Bounds)
	{
		CachedBounds.Expand(B.Bounds.Min);
		CachedBounds.Expand(B.Bounds.Max);
	}

	TArray<FNavigationPrimitiveBounds> NavigationPrimitives;
	CollectNavigationPrimitives(NavigationPrimitives);

	const float Grid = std::max(0.25f, CellSize);
	for (const FBuildBounds& B : Bounds)
	{
		const int32 MinX = static_cast<int32>(floorf(B.Bounds.Min.X / Grid)) - 1;
		const int32 MaxX = static_cast<int32>(ceilf(B.Bounds.Max.X / Grid)) + 1;
		const int32 MinY = static_cast<int32>(floorf(B.Bounds.Min.Y / Grid)) - 1;
		const int32 MaxY = static_cast<int32>(ceilf(B.Bounds.Max.Y / Grid)) + 1;
		const float GuessZ = B.Bounds.GetCenter().Z;

		for (int32 X = MinX; X <= MaxX; ++X)
		{
			for (int32 Y = MinY; Y <= MaxY; ++Y)
			{
				++LastBuildCandidateCount;
				const FGridNavCellKey Key{X, Y};
				if (WalkableCells.find(Key) != WalkableCells.end())
				{
					continue;
				}
				FGridNavCell Cell;
				const FBuildCellResult Result = BuildCell(Key, GuessZ, Bounds, NavigationPrimitives, Cell);
				if (Result.bWalkable)
				{
					WalkableCells[Key] = Cell;
				}
				else if (Result.bBlockedByObstacle)
				{
					BlockedCells[Key] = Result.DebugLocation;
					++BlockedCellCount;
				}
			}
		}
	}

	bNavigationDataBuilt = !WalkableCells.empty();
	const auto BuildEnd = std::chrono::steady_clock::now();
	LastBuildDurationMs = static_cast<float>(std::chrono::duration<double, std::milli>(BuildEnd - BuildStart).count());
	LastBuildMessage = FString("GridNavMesh build ")
		+ (bNavigationDataBuilt ? "succeeded" : "failed")
		+ "; Bounds=" + std::to_string(LastBuildBoundsCount)
		+ "; Candidates=" + std::to_string(LastBuildCandidateCount)
		+ "; NavPrimitives=" + std::to_string(static_cast<int32>(NavigationPrimitives.size()))
		+ "; Walkable=" + std::to_string(GetNavigationNodeCount())
		+ "; Blocked=" + std::to_string(BlockedCellCount)
		+ "; TimeMs=" + std::to_string(LastBuildDurationMs);
	if (bNavigationDataBuilt && bDrawDebugOnBuild)
	{
		DebugDrawNavigationData(DebugDrawDuration);
	}
	return bNavigationDataBuilt;
}

const FGridNavCell* AGridNavMesh::FindCell(const FGridNavCellKey& Key) const
{
	auto It = WalkableCells.find(Key);
	return It != WalkableCells.end() ? &It->second : nullptr;
}

bool AGridNavMesh::FindCellAtPointStrict(const FVector& Point, FGridNavCellKey& OutKey, FVector& OutLocation) const
{
	if (!bNavigationDataBuilt || WalkableCells.empty())
	{
		return false;
	}
	OutKey = WorldToCell(Point);
	const FGridNavCell* Cell = FindCell(OutKey);
	if (!Cell)
	{
		return false;
	}
	OutLocation = Cell->Location;
	return true;
}

bool AGridNavMesh::FindNearestCell(const FVector& Point, FGridNavCellKey& OutKey, FVector& OutLocation) const
{
	if (!bNavigationDataBuilt || WalkableCells.empty())
	{
		return false;
	}
	const FGridNavCellKey Base = WorldToCell(Point);
	float BestDistSq = FLT_MAX;
	bool bFound = false;

	for (int32 Ring = 0; Ring <= std::max(1, NearestCellSearchRings); ++Ring)
	{
		for (int32 DX = -Ring; DX <= Ring; ++DX)
		{
			for (int32 DY = -Ring; DY <= Ring; ++DY)
			{
				if (Ring != 0 && std::abs(DX) != Ring && std::abs(DY) != Ring)
				{
					continue;
				}
				const FGridNavCellKey Key{Base.X + DX, Base.Y + DY};
				const FGridNavCell* Cell = FindCell(Key);
				if (!Cell)
				{
					continue;
				}
				const float DistSq = FVector::DistSquared(Point, Cell->Location);
				if (DistSq < BestDistSq)
				{
					BestDistSq = DistSq;
					OutKey = Key;
					OutLocation = Cell->Location;
					bFound = true;
				}
			}
		}
		if (bFound)
		{
			return true;
		}
	}
	return false;
}

bool AGridNavMesh::ProjectPointToNavigation(const FVector& Point, FVector& OutProjectedPoint, const FNavAgentProperties&) const
{
	FGridNavCellKey Key;
	return FindNearestCell(Point, Key, OutProjectedPoint);
}

bool AGridNavMesh::IsHeightTransitionAllowed(float FromZ, float ToZ, const FNavAgentProperties& AgentProps) const
{
	const float DeltaZ = ToZ - FromZ;
	const float Tolerance = 0.02f;
	if (DeltaZ > 0.0f)
	{
		return DeltaZ <= AgentProps.GetEffectiveMaxClimbHeight() + Tolerance;
	}
	return -DeltaZ <= AgentProps.GetEffectiveMaxDropHeight() + Tolerance;
}

bool AGridNavMesh::HasCachedSegment(const FVector& Start, const FVector& End, const FNavAgentProperties& AgentProps) const
{
	if (!bNavigationDataBuilt || WalkableCells.empty())
	{
		return false;
	}
	const float Dist = sqrtf(HorizontalDistanceSquared(Start, End));
	const float Grid = std::max(0.25f, CellSize);
	const int32 Segments = std::max(1, static_cast<int32>(ceilf(Dist / std::max(0.1f, Grid * 0.5f))));
	FVector Previous = Start;
	FGridNavCellKey PreviousKey;
	bool bHasPreviousKey = false;
	for (int32 Index = 0; Index <= Segments; ++Index)
	{
		const float Alpha = static_cast<float>(Index) / static_cast<float>(Segments);
		const FVector Sample(Start.X + (End.X - Start.X) * Alpha, Start.Y + (End.Y - Start.Y) * Alpha, Start.Z + (End.Z - Start.Z) * Alpha);
		FGridNavCellKey Key;
		FVector CellLocation;
		if (!FindCellAtPointStrict(Sample, Key, CellLocation))
		{
			return false;
		}
		if (bHasPreviousKey)
		{
			const FGridNavCellKey Step{ Key.X - PreviousKey.X, Key.Y - PreviousKey.Y };
			if (std::abs(Step.X) > 1 || std::abs(Step.Y) > 1)
			{
				return false;
			}
			if (!IsDiagonalMoveAllowed(PreviousKey, Step))
			{
				return false;
			}
		}
		if (!IsHeightTransitionAllowed(Previous.Z, CellLocation.Z, AgentProps))
		{
			return false;
		}
		Previous = CellLocation;
		PreviousKey = Key;
		bHasPreviousKey = true;
	}
	return true;
}

bool AGridNavMesh::BuildDirectCachedPath(const FVector& Start, const FVector& End, const FNavAgentProperties& AgentProps, FNavigationPath& OutPath) const
{
	const float Dist = sqrtf(HorizontalDistanceSquared(Start, End));
	const int32 Segments = std::max(1, static_cast<int32>(ceilf(Dist / std::max(0.1f, DirectPathSegmentLength))));
	FVector Previous = Start;
	for (int32 Index = 1; Index <= Segments; ++Index)
	{
		const float Alpha = static_cast<float>(Index) / static_cast<float>(Segments);
		const FVector Sample(Start.X + (End.X - Start.X) * Alpha, Start.Y + (End.Y - Start.Y) * Alpha, Start.Z + (End.Z - Start.Z) * Alpha);
		FGridNavCellKey SampleKey;
		FVector Projected;
		if (!FindCellAtPointStrict(Sample, SampleKey, Projected))
		{
			return false;
		}
		if (!HasCachedSegment(Previous, Projected, AgentProps))
		{
			return false;
		}
		Previous = Projected;
	}
	OutPath.Reset();
	AddPathPointIfDistinct(OutPath, Start);
	AddPathPointIfDistinct(OutPath, End);
	OutPath.bIsPartial = false;
	OutPath.bIsValid = true;
	return true;
}

bool AGridNavMesh::IsDiagonalMoveAllowed(const FGridNavCellKey& From, const FGridNavCellKey& Dir) const
{
	if (Dir.X == 0 || Dir.Y == 0)
	{
		return true;
	}
	const FGridNavCellKey A{From.X + Dir.X, From.Y};
	const FGridNavCellKey B{From.X, From.Y + Dir.Y};
	return FindCell(A) && FindCell(B);
}

bool AGridNavMesh::FindPath(const FVector& Start, const FVector& End, const FNavAgentProperties& AgentProps, FNavigationPath& OutPath) const
{
	OutPath.Reset();
	FGridNavCellKey StartKey;
	FGridNavCellKey GoalKey;
	FVector StartLocation;
	FVector GoalLocation;
	if (!FindNearestCell(Start, StartKey, StartLocation) || !FindNearestCell(End, GoalKey, GoalLocation))
	{
		return false;
	}
	if (BuildDirectCachedPath(StartLocation, GoalLocation, AgentProps, OutPath))
	{
		AddPathPointIfDistinct(OutPath, End);
		return true;
	}

	std::priority_queue<FGridOpenNode> Open;
	std::unordered_map<FGridNavCellKey, FGridSearchRecord, FGridNavCellKeyHash> Records;
	std::unordered_set<FGridNavCellKey, FGridNavCellKeyHash> Closed;

	FGridSearchRecord StartRecord;
	StartRecord.G = 0.0f;
	Records[StartKey] = StartRecord;
	Open.push(FGridOpenNode{StartKey, KeyHeuristic(StartKey, GoalKey), 0.0f});

	const FGridNavCellKey Directions[8] = {
		{ 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 },
		{ 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 }
	};

	FGridNavCellKey BestKey = StartKey;
	float BestH = KeyHeuristic(StartKey, GoalKey);
	bool bReachedGoal = false;
	int32 Visited = 0;

	while (!Open.empty() && Visited < MaxSearchNodes)
	{
		const FGridOpenNode Current = Open.top();
		Open.pop();
		if (Closed.find(Current.Key) != Closed.end())
		{
			continue;
		}
		Closed.insert(Current.Key);
		++Visited;

		const float H = KeyHeuristic(Current.Key, GoalKey);
		if (H < BestH)
		{
			BestH = H;
			BestKey = Current.Key;
		}
		if (Current.Key == GoalKey)
		{
			BestKey = Current.Key;
			bReachedGoal = true;
			break;
		}

		const FGridNavCell* CurrentCell = FindCell(Current.Key);
		if (!CurrentCell)
		{
			continue;
		}

		for (const FGridNavCellKey& Dir : Directions)
		{
			if (!IsDiagonalMoveAllowed(Current.Key, Dir))
			{
				continue;
			}
			const FGridNavCellKey Next{Current.Key.X + Dir.X, Current.Key.Y + Dir.Y};
			if (Closed.find(Next) != Closed.end())
			{
				continue;
			}
			const FGridNavCell* NextCell = FindCell(Next);
			if (!NextCell)
			{
				continue;
			}
			if (!HasCachedSegment(CurrentCell->Location, NextCell->Location, AgentProps))
			{
				continue;
			}

			const float StepCost = (Dir.X != 0 && Dir.Y != 0) ? 1.41421356f : 1.0f;
			const float NewG = Records[Current.Key].G + StepCost;
			auto Existing = Records.find(Next);
			if (Existing != Records.end() && NewG >= Existing->second.G)
			{
				continue;
			}

			FGridSearchRecord Record;
			Record.Parent = Current.Key;
			Record.G = NewG;
			Record.bHasParent = true;
			Records[Next] = Record;
			Open.push(FGridOpenNode{Next, NewG + KeyHeuristic(Next, GoalKey), NewG});
		}
	}

	if (Records.find(BestKey) == Records.end())
	{
		return false;
	}

	TArray<FGridNavCellKey> ReversedKeys;
	FGridNavCellKey Cursor = BestKey;
	for (int32 Guard = 0; Guard < MaxSearchNodes; ++Guard)
	{
		ReversedKeys.push_back(Cursor);
		if (Cursor == StartKey)
		{
			break;
		}
		auto It = Records.find(Cursor);
		if (It == Records.end() || !It->second.bHasParent)
		{
			break;
		}
		Cursor = It->second.Parent;
	}
	if (ReversedKeys.empty() || !(ReversedKeys.back() == StartKey))
	{
		return false;
	}

	OutPath.Reset();
	AddPathPointIfDistinct(OutPath, StartLocation);
	for (auto It = ReversedKeys.rbegin(); It != ReversedKeys.rend(); ++It)
	{
		const FGridNavCell* Cell = FindCell(*It);
		if (Cell)
		{
			AddPathPointIfDistinct(OutPath, Cell->Location);
		}
	}
	if (bReachedGoal)
	{
		AddPathPointIfDistinct(OutPath, GoalLocation);
		AddPathPointIfDistinct(OutPath, End);
	}
	OutPath.bIsPartial = !bReachedGoal;
	OutPath.bIsValid = !OutPath.PathPoints.empty();
	SmoothPath(AgentProps, OutPath);
	return OutPath.IsValid();
}

bool AGridNavMesh::NavigationRaycast(const FVector& Start, const FVector& End, const FNavAgentProperties& AgentProps) const
{
	FVector ProjectedStart;
	FVector ProjectedEnd;
	if (!ProjectPointToNavigation(Start, ProjectedStart, AgentProps) || !ProjectPointToNavigation(End, ProjectedEnd, AgentProps))
	{
		return true;
	}
	FNavigationPath DirectPath;
	return !BuildDirectCachedPath(ProjectedStart, ProjectedEnd, AgentProps, DirectPath);
}

bool AGridNavMesh::GetRandomReachablePointInRadius(const FVector& Origin, float Radius, FVector& OutPoint, const FNavAgentProperties& AgentProps) const
{
	if (!bNavigationDataBuilt || WalkableCells.empty())
	{
		return false;
	}
	FVector ProjectedOrigin;
	FGridNavCellKey OriginKey;
	if (!FindNearestCell(Origin, OriginKey, ProjectedOrigin))
	{
		return false;
	}

	const float RadiusSq = Radius * Radius;
	FVector Best = ProjectedOrigin;
	float BestDistSq = -1.0f;
	int32 Considered = 0;
	for (const auto& Pair : WalkableCells)
	{
		const FVector& Candidate = Pair.second.Location;
		const float DistSq = HorizontalDistanceSquared(ProjectedOrigin, Candidate);
		if (DistSq > RadiusSq)
		{
			continue;
		}
		++Considered;
		// Deterministic pseudo-random spread without depending on global RNG state.
		const uint32 Hash = static_cast<uint32>((Pair.first.X * 73856093) ^ (Pair.first.Y * 19349663));
		const float Score = static_cast<float>(Hash & 0xffff) + DistSq * 0.001f;
		if (Score > BestDistSq)
		{
			FNavigationPath Path;
			if (FindPath(ProjectedOrigin, Candidate, AgentProps, Path) && !Path.bIsPartial)
			{
				BestDistSq = Score;
				Best = Candidate;
			}
		}
	}
	OutPoint = Considered > 0 ? Best : ProjectedOrigin;
	return true;
}

bool AGridNavMesh::IsLocationOnNavData(const FVector& Location) const
{
	FGridNavCellKey Key;
	FVector Projected;
	return FindNearestCell(Location, Key, Projected) && FVector::DistSquared(Location, Projected) <= CellSize * CellSize;
}

void AGridNavMesh::SmoothPath(const FNavAgentProperties& AgentProps, FNavigationPath& InOutPath) const
{
	if (InOutPath.PathPoints.size() <= 2)
	{
		return;
	}
	TArray<FNavPathPoint> Smoothed;
	size_t Current = 0;
	Smoothed.push_back(InOutPath.PathPoints[0]);
	while (Current + 1 < InOutPath.PathPoints.size())
	{
		size_t Best = Current + 1;
		for (size_t Candidate = InOutPath.PathPoints.size() - 1; Candidate > Current + 1; --Candidate)
		{
			if (HasCachedSegment(InOutPath.PathPoints[Current].Location, InOutPath.PathPoints[Candidate].Location, AgentProps))
			{
				Best = Candidate;
				break;
			}
		}
		Smoothed.push_back(InOutPath.PathPoints[Best]);
		Current = Best;
	}
	InOutPath.PathPoints = Smoothed;
}

FColor AGridNavMesh::GetHeightDebugColor(float HeightZ, float MinZ, float MaxZ) const
{
	if (!bDrawHeightColors || MaxZ <= MinZ + 0.001f)
	{
		return FColor(40, 180, 80);
	}

	const float T = Clamp01((HeightZ - MinZ) / (MaxZ - MinZ));
	// Low to high: deep blue -> cyan -> green -> yellow -> red.  This makes
	// walkable surfaces on top of walls/boxes immediately stand out from floor cells.
	if (T < 0.25f)
	{
		return LerpColor(FColor(45, 90, 220), FColor(40, 210, 220), T / 0.25f);
	}
	if (T < 0.50f)
	{
		return LerpColor(FColor(40, 210, 220), FColor(40, 190, 80), (T - 0.25f) / 0.25f);
	}
	if (T < 0.75f)
	{
		return LerpColor(FColor(40, 190, 80), FColor(230, 220, 45), (T - 0.50f) / 0.25f);
	}
	return LerpColor(FColor(230, 220, 45), FColor(230, 60, 45), (T - 0.75f) / 0.25f);
}

bool AGridNavMesh::ShouldDrawHeightContour(float HeightZ, float MinZ) const
{
	if (!bDrawHeightColors || !bDrawHeightContours || DebugHeightContourInterval <= 0.001f)
	{
		return false;
	}
	const float LocalHeight = HeightZ - MinZ;
	const float Interval = std::max(0.001f, DebugHeightContourInterval);
	const float Mod = fmodf(fabsf(LocalHeight), Interval);
	const float DistanceToLine = std::min(Mod, Interval - Mod);
	return DistanceToLine <= std::max(0.015f, Interval * 0.06f);
}

void AGridNavMesh::DebugDrawNavigationData(float Duration) const
{
	UWorld* World = GetWorld();
	if (!World || !bNavigationDataBuilt)
	{
		return;
	}

	FDebugDrawQueue& Queue = World->GetScene().GetDebugDrawQueue();
	Queue.ClearCategory(EDebugDrawCategory::Navigation);

	const float Grid = std::max(0.25f, CellSize);
	const float Half = Grid * 0.42f;
	const int32 TotalCells = static_cast<int32>(WalkableCells.size() + (bDrawBlockedCells ? BlockedCells.size() : 0));
	const int32 Stride = ComputeDebugDrawStride(TotalCells, std::max(0, DebugDrawMaxCells));

	float MinHeight = FLT_MAX;
	float MaxHeight = -FLT_MAX;
	for (const auto& Pair : WalkableCells)
	{
		MinHeight = std::min(MinHeight, Pair.second.Location.Z);
		MaxHeight = std::max(MaxHeight, Pair.second.Location.Z);
	}
	if (WalkableCells.empty())
	{
		MinHeight = 0.0f;
		MaxHeight = 0.0f;
	}

	auto AddNavLine = [&Queue, Duration](const FVector& A, const FVector& B, const FColor& Color)
	{
		Queue.AddLine(A, B, Color, Duration, EDebugDrawCategory::Navigation);
	};

	auto AddNavBox = [&Queue, Duration](const FVector& Center, const FVector& Extent, const FColor& Color)
	{
		Queue.AddBox(Center, Extent, Color, Duration, EDebugDrawCategory::Navigation);
	};

	auto DrawCellSquare = [&AddNavLine, Half](const FVector& Center, const FColor& Color)
	{
		const FVector A = Center + FVector(-Half, -Half, 0.0f);
		const FVector B = Center + FVector(Half, -Half, 0.0f);
		const FVector C = Center + FVector(Half, Half, 0.0f);
		const FVector D = Center + FVector(-Half, Half, 0.0f);
		AddNavLine(A, B, Color);
		AddNavLine(B, C, Color);
		AddNavLine(C, D, Color);
		AddNavLine(D, A, Color);
	};

	auto DrawContourMark = [&AddNavLine, Half](const FVector& Center)
	{
		const float Inner = Half * 0.55f;
		AddNavLine(Center + FVector(-Inner, 0.0f, 0.0f), Center + FVector(Inner, 0.0f, 0.0f), FColor(245, 245, 245));
		AddNavLine(Center + FVector(0.0f, -Inner, 0.0f), Center + FVector(0.0f, Inner, 0.0f), FColor(245, 245, 245));
	};

	if (CachedBounds.IsValid())
	{
		AddNavBox(CachedBounds.GetCenter(), CachedBounds.GetExtent(), FColor(80, 160, 255));
	}

	int32 DrawIndex = 0;
	for (const auto& Pair : WalkableCells)
	{
		if ((DrawIndex++ % Stride) != 0)
		{
			continue;
		}
		const FVector Center = Pair.second.Location + FVector(0.0f, 0.0f, 4.0f);
		DrawCellSquare(Center, GetHeightDebugColor(Pair.second.Location.Z, MinHeight, MaxHeight));
		if (ShouldDrawHeightContour(Pair.second.Location.Z, MinHeight))
		{
			DrawContourMark(Center + FVector(0.0f, 0.0f, 0.02f));
		}
	}

	// Blocked cells must be drawn last and above the walkable grid.  Otherwise an
	// obstacle footprint can be correctly blocked but still look green because a
	// nearby walkable surface line is drawn over it.
	if (bDrawBlockedCells)
	{
		for (const auto& Pair : BlockedCells)
		{
			if ((DrawIndex++ % Stride) != 0)
			{
				continue;
			}
			const FVector Center = Pair.second + FVector(0.0f, 0.0f, 7.0f);
			AddNavLine(Center + FVector(-Half, -Half, 0.0f), Center + FVector(Half, Half, 0.0f), FColor(230, 35, 35));
			AddNavLine(Center + FVector(Half, -Half, 0.0f), Center + FVector(-Half, Half, 0.0f), FColor(230, 35, 35));
			DrawCellSquare(Center, FColor(180, 30, 30));
		}
	}
}
