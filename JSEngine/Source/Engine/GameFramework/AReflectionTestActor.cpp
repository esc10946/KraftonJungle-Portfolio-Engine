#include "AReflectionTestActor.h"

#include "NewTestComponent.h"

void AReflectionTestActor::InitDefaultComponents()
{
    auto root = AddComponent<USceneComponent>();
    SetRootComponent(root);

	UClass* NewClass = NewTestComponent::StaticClass();
}
