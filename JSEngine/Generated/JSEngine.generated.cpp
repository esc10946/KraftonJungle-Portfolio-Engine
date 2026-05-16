// This file is generated. Do not edit manually.

#include "../Source/Engine/GameFramework/AReflectionTestActor.h"

#include "Object/Class.h"
#include "Object/ObjectFactory.h"
#include "Object/ReflectionRegistry.h"

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
