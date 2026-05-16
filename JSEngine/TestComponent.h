#pragma once
#include "Component/ActorComponent.h"
#include "TestComponent.generated.h"

UCLASS()
class TestComponent : public UActorComponent
{
    GENERATED_BODY(TestComponent, UActorComponent)
public:
    UPROPERTY(Edit, LuaRead, LuaWrite)
    float Speed = 0.0f;

    UPROPERTY(Edit)
    bool bEnabled = true;

    UPROPERTY(Edit)
    FStaticMeshAssetRef WalkAnimation;

    UPROPERTY(Edit)
    TArray<float> Values;
};
