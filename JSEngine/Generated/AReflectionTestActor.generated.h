#pragma once

class AReflectionTestActor;

template<>
struct TIsUClassReflected<AReflectionTestActor>
{
    static constexpr bool Value = true;
};

#define AREFLECTIONTESTACTOR_GENERATED_BODY() \
public: \
    using ThisClass = AReflectionTestActor; \
    using Super = AActor; \
    static UClass* StaticClass(); \
    UClass* GetClass() const override { return StaticClass(); }
