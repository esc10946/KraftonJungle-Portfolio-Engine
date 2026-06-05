#include "GameFramework/Actor/NavMeshBoundsVolume.h"
#include "Component/Shape/BoxComponent.h"
#include "GameFramework/World.h"
#include "Navigation/NavigationSystem.h"

void ANavMeshBoundsVolume::InitDefaultComponents(const FVector& Extent)
{
	BoundsComponent = AddComponent<UBoxComponent>();
	if (BoundsComponent)
	{
		BoundsComponent->SetBoxExtent(Extent);
		BoundsComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetRootComponent(BoundsComponent);
	}
}

void ANavMeshBoundsVolume::PostDuplicate()
{
	Super::PostDuplicate();
	BoundsComponent = GetComponentByClass<UBoxComponent>();
}

bool ANavMeshBoundsVolume::ContainsPoint(const FVector& Point) const
{
	UBoxComponent* Box = BoundsComponent.Get();
	if (!Box)
	{
		return false;
	}
	const FVector Center = Box->GetWorldLocation();
	const FVector Extent = Box->GetScaledBoxExtent();
	return Point.X >= Center.X - Extent.X && Point.X <= Center.X + Extent.X
		&& Point.Y >= Center.Y - Extent.Y && Point.Y <= Center.Y + Extent.Y
		&& Point.Z >= Center.Z - Extent.Z && Point.Z <= Center.Z + Extent.Z;
}

void ANavMeshBoundsVolume::PostEditProperty(const char* PropertyName)
{
	Super::PostEditProperty(PropertyName);
	if (UWorld* World = GetWorld())
	{
		if (UNavigationSystem* NavSys = World->GetNavigationSystem())
		{
			NavSys->InvalidateNavigationData();
		}
	}
}
