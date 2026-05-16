#pragma once

#include "Core/CoreMinimal.h"

// These macros wrap metadata parsed by the Unreal Header Tool, and are otherwise
// ignored when code containing them is compiled by the C++ compiler
#define UPROPERTY(...)
#define UFUNCTION(...)
#define USTRUCT(...)
#define UMETA(...)
#define UPARAM(...)
#define UENUM(...)
#define UDELEGATE(...)
#define RIGVM_METHOD(...)
#define VMODULE(...)

#define GENERATED_BODY(ClassName, SuperClass) \
public:                                       \
    using ThisClass = ClassName;              \
    using Super = SuperClass;                 \
    static UClass* StaticClass();             \
    UClass* GetClass() const override         \
    {                                         \
        return StaticClass();                 \
    }