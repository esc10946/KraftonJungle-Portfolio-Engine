// This file is generated. Do not edit manually.

#include "../NewTestComponent.h"
#include "../Source/Engine/GameFramework/AReflectionTestActor.h"

#include "Object/Class.h"
#include "Object/ObjectFactory.h"
#include "Object/ReflectionRegistry.h"

UClass* NewTestComponent::StaticClass()
{
    static UClass Class(
        "NewTestComponent",
        Super::StaticClass(),
        sizeof(NewTestComponent),
        CF_Component,
        []() -> UObject*
        {
            return UObjectManager::Get().CreateObject<NewTestComponent>();
        });

    static bool bRegistered = false;
    if (!bRegistered)
    {
        bRegistered = true;

        Class.AddProperty({ "Speed",
                            EReflectedPropertyType::Float,
                            offsetof(NewTestComponent, Speed),
                            sizeof(float),
                            EPropertyFlags::Read |
                                EPropertyFlags::Write |
                                EPropertyFlags::Edit |
                                EPropertyFlags::LuaRead |
                                EPropertyFlags::LuaWrite,
                            nullptr });

        Class.AddProperty({ "bActive",
                            EReflectedPropertyType::Bool,
                            offsetof(NewTestComponent, bActive),
                            sizeof(bool),
                            EPropertyFlags::Read |
                                EPropertyFlags::Write |
                                EPropertyFlags::Edit |
                                EPropertyFlags::LuaRead |
                                EPropertyFlags::LuaWrite,
                            nullptr });

        Class.AddProperty({ "Name",
                            EReflectedPropertyType::Name,
                            offsetof(NewTestComponent, Name),
                            sizeof(FName),
                            EPropertyFlags::Read |
                                EPropertyFlags::Write |
                                EPropertyFlags::Edit |
                                EPropertyFlags::LuaRead |
                                EPropertyFlags::LuaWrite,
                            nullptr });

        Class.AddProperty({ "Index",
                            EReflectedPropertyType::Int32,
                            offsetof(NewTestComponent, Index),
                            sizeof(int32),
                            EPropertyFlags::LuaRead |
                                EPropertyFlags::LuaWrite,
                            nullptr });

        Class.AddProperty({ "ObjectPath",
                            EReflectedPropertyType::String,
                            offsetof(NewTestComponent, ObjectPath),
                            sizeof(FString),
                            EPropertyFlags::Read |
                                EPropertyFlags::Write |
                                EPropertyFlags::Edit |
                                EPropertyFlags::LuaRead |
                                EPropertyFlags::LuaWrite,
                            nullptr });

        Class.AddProperty({ "Object",
                            EReflectedPropertyType::Object,
                            offsetof(NewTestComponent, Object),
                            sizeof(USceneComponent*),
                            EPropertyFlags::Read |
                                EPropertyFlags::Write |
                                EPropertyFlags::Edit |
                                EPropertyFlags::LuaRead |
                                EPropertyFlags::LuaWrite,
                            USceneComponent::StaticClass() });

        FReflectionRegistry::Get().RegisterUClass(&Class);
    }

    return &Class;
}

namespace
{
    struct FAutoRegisterNewTestComponent
    {
        FAutoRegisterNewTestComponent()
        {
            NewTestComponent::StaticClass();
        }
    };

    static FAutoRegisterNewTestComponent GAutoRegisterNewTestComponent;
}

UClass* AReflectionTestActor::StaticClass()
{
    static UClass Class(
        "AReflectionTestActor",
        Super::StaticClass(),
        sizeof(AReflectionTestActor),
        CF_Actor,
        []() -> UObject*
        {
            return UObjectManager::Get().CreateObject<AReflectionTestActor>();
        });

    static bool bRegistered = false;
    if (!bRegistered)
    {
        bRegistered = true;

        Class.AddProperty({ "Speed",
                            EReflectedPropertyType::Float,
                            offsetof(AReflectionTestActor, Speed),
                            sizeof(float),
                            EPropertyFlags::Read |
                                EPropertyFlags::Write |
                                EPropertyFlags::Edit |
                                EPropertyFlags::LuaRead |
                                EPropertyFlags::LuaWrite,
                            nullptr });

        Class.AddProperty({ "Health",
                            EReflectedPropertyType::Float,
                            offsetof(AReflectionTestActor, Health),
                            sizeof(float),
                            EPropertyFlags::Read |
                                EPropertyFlags::Write |
                                EPropertyFlags::Edit |
                                EPropertyFlags::LuaRead |
                                EPropertyFlags::LuaWrite,
                            nullptr });

        FReflectionRegistry::Get().RegisterUClass(&Class);
    }

    return &Class;
}

namespace
{
    struct FAutoRegisterAReflectionTestActor
    {
        FAutoRegisterAReflectionTestActor()
        {
            AReflectionTestActor::StaticClass();
        }
    };

    static FAutoRegisterAReflectionTestActor GAutoRegisterAReflectionTestActor;
}
