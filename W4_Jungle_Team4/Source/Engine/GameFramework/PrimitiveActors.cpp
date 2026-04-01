#include "GameFramework/PrimitiveActors.h"

#include "Component/PrimitiveComponent.h"
#include "Component/StaticMeshComponent.h"
#include "Component/TextRenderComponent.h"
#include <Component/SubUVComponent.h>
#include "Engine/Asset/Material/MaterialManager.h"
#include "Engine/Asset/Material/MaterialObject.h"
#include "Engine/Asset/StaticMesh/StaticMeshManager.h"
#include "Engine/Asset/StaticMesh/StaticMeshObject.h"
#include "Engine/Core/Paths.h"

#include <format>

DEFINE_CLASS(ACubeActor, AStaticMeshActor)
REGISTER_FACTORY(ACubeActor)

DEFINE_CLASS(ASphereActor, AStaticMeshActor)
REGISTER_FACTORY(ASphereActor)

DEFINE_CLASS(APlaneActor, AStaticMeshActor)
REGISTER_FACTORY(APlaneActor)

DEFINE_CLASS(AStaticMeshActor, AActor)
REGISTER_FACTORY(AStaticMeshActor)

DEFINE_CLASS(AAttachTestActor, AActor)
REGISTER_FACTORY(AAttachTestActor)


void AStaticMeshActor::InitDefaultComponents()
{
	auto* StaticMesh = AddComponent<UStaticMeshComponent>();
	SetRootComponent(StaticMesh);

	const FString StaticMeshPath = FPaths::ToUtf8(FPaths::MeshPath()) + "cube-tex.smbin";
	StaticMesh->SetStaticMesh(FStaticMeshManager::Get().LoadStaticMeshObject(StaticMeshPath));

	auto* Text = AddComponent<UTextRenderComponent>();
	Text->SetFont(FName("Default"));
	Text->SetText(std::format("UUID: {}", GetUUID()));
	Text->AttachToComponent(StaticMesh);
	UpdateUUIDTextLocation();
}

void AStaticMeshActor::UpdateUUIDTextLocation()
{
	auto* StaticMeshComponent = UStaticMeshComponent::Cast(GetRootComponent());
	if (!StaticMeshComponent)
	{
		return;
	}

	UTextRenderComponent* TextComponent = nullptr;
	for (UActorComponent* Component : GetComponents())
	{
		TextComponent = UTextRenderComponent::Cast(Component);
		if (TextComponent)
		{
			break;
		}
	}

	if (!TextComponent)
	{
		return;
	}

	UStaticMesh* Mesh = StaticMeshComponent->GetStaticMesh();
	if (!Mesh)
	{
		return;
	}

	const FVector Min = Mesh->GetBoundsMin();
	const FVector Max = Mesh->GetBoundsMax();
	const float Padding = 0.1f;
	const float TextHalfHeight = 0.25f * TextComponent->GetFontSize() * TextComponent->GetRelativeScale().Z;

	TextComponent->SetRelativeLocation(FVector(
		(Min.X + Max.X) * 0.5f,
		(Min.Y + Max.Y) * 0.5f,
		Max.Z + Padding + TextHalfHeight));
}

void ACubeActor::InitDefaultComponents()
{
	UStaticMeshComponent* CubeMesh = AddComponent<UStaticMeshComponent>();
	SetRootComponent(CubeMesh);
	
	const FString StaticMeshPath = FPaths::ToUtf8(FPaths::MeshPath()) + "cube.obj";
	CubeMesh->SetStaticMesh(FStaticMeshManager::Get().LoadStaticMeshObject(StaticMeshPath));

	// Text
	UTextRenderComponent* Text = AddComponent<UTextRenderComponent>();
	Text->SetFont(FName("Default"));
	Text->AttachToComponent(CubeMesh);
	Text->SetText("UUID : " + std::to_string(GetUUID()));
	UpdateUUIDTextLocation();
}

void ASphereActor::InitDefaultComponents()
{
	UStaticMeshComponent* SphereMesh = AddComponent<UStaticMeshComponent>();
	SetRootComponent(SphereMesh);

	const FString StaticMeshPath = FPaths::ToUtf8(FPaths::MeshPath()) + "sphere.obj";
	SphereMesh->SetStaticMesh(FStaticMeshManager::Get().LoadStaticMeshObject(StaticMeshPath));

	UTextRenderComponent* Text = AddComponent<UTextRenderComponent>();
	Text->SetFont(FName("Default"));
	Text->AttachToComponent(SphereMesh);
	Text->SetText("UUID : " + std::to_string(GetUUID()));
	UpdateUUIDTextLocation();
}

void APlaneActor::InitDefaultComponents()
{
	UStaticMeshComponent* PlaneMesh = AddComponent<UStaticMeshComponent>();
	SetRootComponent(PlaneMesh);

	const FString StaticMeshPath = FPaths::ToUtf8(FPaths::MeshPath()) + "plane.obj";
	PlaneMesh->SetStaticMesh(FStaticMeshManager::Get().LoadStaticMeshObject(StaticMeshPath));

	UTextRenderComponent* Text = AddComponent<UTextRenderComponent>();
	Text->SetFont(FName("Default"));
	Text->SetText(std::format("UUID: {}", GetUUID()));
	Text->AttachToComponent(PlaneMesh);
	UpdateUUIDTextLocation();

	// SubUV
	USubUVComponent* SubUV = AddComponent<USubUVComponent>();
	SubUV->AttachToComponent(PlaneMesh);
	SubUV->SetParticle(FName("Explosion"));
	SubUV->SetSpriteSize(2.0f, 2.0f);
	SubUV->SetFrameRate(30.f);
	SubUV->SetRelativeLocation(FVector(0.0f, 0.0f, 2.3f));
}

void AAttachTestActor::InitDefaultComponents()
{
	// Root: Cube
	auto* Cube = AddComponent<UCubeComponent>();
	SetRootComponent(Cube);

	// Grouping node for spheres
	auto* Primitives = AddComponent<USceneComponent>();
	Primitives->AttachToComponent(Cube);

	// 4 Spheres in a square pattern
	constexpr float Offset = 2.0f;
	const FVector Positions[4] = {
		{ -Offset, -Offset, 0.0f },
		{  Offset, -Offset, 0.0f },
		{  Offset,  Offset, 0.0f },
		{ -Offset,  Offset, 0.0f },
	};
	for (int i = 0; i < 4; ++i)
	{
		auto* Sphere = AddComponent<USphereComponent>();
		Sphere->AttachToComponent(Primitives);
		Sphere->SetRelativeLocation(Positions[i]);
	}

	// Text attached directly to Root
	auto* Text = AddComponent<UTextRenderComponent>();
	Text->AttachToComponent(Cube);
	Text->SetText("UUID : " + std::to_string(GetUUID()));
	Text->SetRelativeLocation(FVector(0.0f, 0.0f, 1.5f));
}
