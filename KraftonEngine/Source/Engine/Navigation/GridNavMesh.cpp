#include "Navigation/GridNavMesh.h"

#include "Component/PrimitiveComponent.h"
#include "Component/Shape/BoxComponent.h"
#include "Debug/DrawDebugHelpers.h"
#include "GameFramework/Actor/NavMeshBoundsVolume.h"
#include "GameFramework/World.h"
#include "Math/MathUtils.h"
#include "Math/Quat.h"
#include "Navigation/NavigationSystem.h"

#include <algorithm>
#include <chrono>
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
}

AGridNavMesh::AGridNavMesh()
{
	bNeedsTick = false;
}

UNavigationSystem* AGridNavMesh::GetNavigationSystem() const
{
	UWorld* World = GetWorld();
	return World ? World->GetNavigationSystem() : nullptr;
}

void AGridNavMesh::SetCellSize(float InCellSize)
{
	CellSize = std::max(0.5f, InCellSize);
	bNavigationDataBuilt = false;
}

void AGridNavMesh::ClearNavigationData()
{
	WalkableCells.clear();
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
			if (!Primitive || !Primitive->IsQueryCollisionEnabled() || Primitive->GetCollisionObjectType() != ECollisionChannel::WorldStatic)
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

FGridNavCellKey AGridNavMesh::WorldToCell(const FVector& Point) const
{
	const float Grid = std::max(0.5f, CellSize);
	return FGridNavCellKey{
		static_cast<int32>(floorf(Point.X / Grid)),
		static_cast<int32>(floorf(Point.Y / Grid))
	};
}

FVector AGridNavMesh::CellToWorldGuess(const FGridNavCellKey& Key, float Z) const
{
	const float Grid = std::max(0.5f, CellSize);
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

bool AGridNavMesh::ProjectPointToWalkableSurfaceByPrimitiveBounds(const FVector& Guess, const TArray<FBuildBounds>& Bounds, FVector& OutProjectedLocation) const
{
	UWorld* World = GetWorld();
	if (!World || !IsInsideAnyBuildBounds(Guess, Bounds))
	{
		return false;
	}

	const float HalfCell = std::max(0.05f, CellSize * 0.5f);
	const float StartZ = Guess.Z + ProjectionUp;
	const float EndZ = Guess.Z - ProjectionDown;
	const float MaxFloorThickness = std::max(0.01f, WalkableFloorMaxThickness);
	bool bFound = false;
	float BestZ = FLT_MAX;

	for (AActor* Actor : World->GetActors())
	{
		if (!Actor || Cast<ANavMeshBoundsVolume>(Actor))
		{
			continue;
		}
		for (UPrimitiveComponent* Primitive : Actor->GetPrimitiveComponents())
		{
			if (!Primitive || !Primitive->IsQueryCollisionEnabled() || Primitive->GetCollisionObjectType() != ECollisionChannel::WorldStatic)
			{
				continue;
			}
			const FBoundingBox B = Primitive->GetWorldBoundingBox();
			if (!B.IsValid())
			{
				continue;
			}
			const FVector Extent = B.GetExtent();
			// Box/mesh bounds that are tall compared to the agent step are treated as
			// obstacles, not walkable floor tops. This prevents pillars/walls from
			// becoming accidental nav surfaces in small test scenes.
			if (Extent.Z > MaxFloorThickness)
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
			if (TopZ < BestZ)
			{
				BestZ = TopZ;
				bFound = true;
			}
		}
	}

	if (!bFound)
	{
		return false;
	}
	OutProjectedLocation = FVector(Guess.X, Guess.Y, BestZ);
	return IsInsideAnyBuildBounds(OutProjectedLocation, Bounds);
}

bool AGridNavMesh::HasAgentFootprintByPrimitiveBounds(const FVector& ProjectedLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const float Radius = std::max(0.05f, SupportedAgent.Radius);
	const float AgentBottom = ProjectedLocation.Z + 0.05f;
	const float AgentTop = ProjectedLocation.Z + std::max(Radius * 2.0f, SupportedAgent.Height);

	for (AActor* Actor : World->GetActors())
	{
		if (!Actor || Cast<ANavMeshBoundsVolume>(Actor))
		{
			continue;
		}
		for (UPrimitiveComponent* Primitive : Actor->GetPrimitiveComponents())
		{
			if (!Primitive || !Primitive->IsQueryCollisionEnabled() || Primitive->GetCollisionObjectType() != ECollisionChannel::WorldStatic)
			{
				continue;
			}
			const FBoundingBox B = Primitive->GetWorldBoundingBox();
			if (!B.IsValid())
			{
				continue;
			}
			// The supporting floor ends at or below the projected floor height; it
			// should not block its own walkable cell.
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
	}
	return true;
}

bool AGridNavMesh::HasAgentFootprint(const FVector& ProjectedLocation) const
{
	if (!HasAgentFootprintByPrimitiveBounds(ProjectedLocation))
	{
		return false;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const float Radius = std::max(0.05f, SupportedAgent.Radius);
	const float HalfHeight = std::max(Radius, SupportedAgent.Height * 0.5f);
	const FVector Center(ProjectedLocation.X, ProjectedLocation.Y, ProjectedLocation.Z + HalfHeight);
	const FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
	FHitResult Hit;
	if (!World->PhysicsSweepByObjectTypes(Center, Center + FVector(0.0f, 0.0f, 0.1f), FQuat(), Shape, Hit, ObjectTypeBit(ECollisionChannel::WorldStatic)))
	{
		return true;
	}

	// Physics sweep can report the supporting floor as an initial overlap on
	// simple mesh collision.  Bounds-based clearance above already filtered real
	// blockers, so do not fail the cell only because of that conservative hit.
	return true;
}

bool AGridNavMesh::BuildCell(const FGridNavCellKey& Key, float GuessZ, const TArray<FBuildBounds>& Bounds, FGridNavCell& OutCell) const
{
	UNavigationSystem* NavSys = GetNavigationSystem();
	if (!NavSys)
	{
		return false;
	}
	const FVector Guess = CellToWorldGuess(Key, GuessZ);
	if (!IsInsideAnyBuildBounds(Guess, Bounds))
	{
		return false;
	}
	FVector Projected;
	if (!ProjectPointToWalkableSurfaceByPrimitiveBounds(Guess, Bounds, Projected) &&
		!NavSys->ProjectPointToNavigationRuntime(Guess, Projected, SupportedAgent, ProjectionUp, ProjectionDown))
	{
		return false;
	}
	if (!IsInsideAnyBuildBounds(Projected, Bounds))
	{
		return false;
	}
	if (!HasAgentFootprint(Projected))
	{
		return false;
	}
	OutCell.Location = Projected;
	OutCell.bWalkable = true;
	return true;
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

	const float Grid = std::max(0.5f, CellSize);
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
				if (BuildCell(Key, GuessZ, Bounds, Cell))
				{
					WalkableCells[Key] = Cell;
				}
				else
				{
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

bool AGridNavMesh::HasCachedSegment(const FVector& Start, const FVector& End, const FNavAgentProperties& AgentProps) const
{
	if (!bNavigationDataBuilt || WalkableCells.empty())
	{
		return false;
	}
	const float Dist = sqrtf(HorizontalDistanceSquared(Start, End));
	const float Grid = std::max(0.5f, CellSize);
	const int32 Segments = std::max(1, static_cast<int32>(ceilf(Dist / std::max(0.1f, Grid * 0.5f))));
	FVector Previous = Start;
	for (int32 Index = 0; Index <= Segments; ++Index)
	{
		const float Alpha = static_cast<float>(Index) / static_cast<float>(Segments);
		const FVector Sample(Start.X + (End.X - Start.X) * Alpha, Start.Y + (End.Y - Start.Y) * Alpha, Start.Z + (End.Z - Start.Z) * Alpha);
		FGridNavCellKey Key;
		FVector CellLocation;
		if (!FindNearestCell(Sample, Key, CellLocation))
		{
			return false;
		}
		if (HorizontalDistanceSquared(Sample, CellLocation) > (Grid * 1.5f) * (Grid * 1.5f))
		{
			return false;
		}
		if (fabsf(CellLocation.Z - Previous.Z) > std::max(AgentProps.StepHeight, Grid * 0.5f))
		{
			return false;
		}
		Previous = CellLocation;
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
		FVector Projected;
		if (!ProjectPointToNavigation(Sample, Projected, AgentProps))
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

void AGridNavMesh::DebugDrawNavigationData(float Duration) const
{
	UWorld* World = GetWorld();
	if (!World || !bNavigationDataBuilt)
	{
		return;
	}
	const float Half = std::max(0.5f, CellSize) * 0.42f;
	for (const auto& Pair : WalkableCells)
	{
		const FVector Center = Pair.second.Location + FVector(0.0f, 0.0f, 4.0f);
		DrawDebugLine(World, Center + FVector(-Half, -Half, 0.0f), Center + FVector(Half, -Half, 0.0f), FColor(40, 180, 80), Duration);
		DrawDebugLine(World, Center + FVector(Half, -Half, 0.0f), Center + FVector(Half, Half, 0.0f), FColor(40, 180, 80), Duration);
		DrawDebugLine(World, Center + FVector(Half, Half, 0.0f), Center + FVector(-Half, Half, 0.0f), FColor(40, 180, 80), Duration);
		DrawDebugLine(World, Center + FVector(-Half, Half, 0.0f), Center + FVector(-Half, -Half, 0.0f), FColor(40, 180, 80), Duration);
	}
}
