#pragma once
#include "Core/CoreMinimal.h"
#include "Property.h"

class UObject;
class UClass
{
public:
    using FCreateObjectFunc = std::function<UObject*()>;

    UClass(
        const char* InName,
        UClass* InSuperClass,
        size_t InClassSize,
        uint32 InClassFlags,
        FCreateObjectFunc InCreateFunc);

    const char* GetName() const { return Name; }
    UClass* GetSuperClass() const { return SuperClass; }
    size_t GetClassSize() const { return ClassSize; }
    uint32 GetClassFlags() const { return ClassFlags; }

    bool IsChildOf(const UClass* Other) const;

    UObject* CreateObject() const;
    void GetAllProperties(TArray<const FProperty*>& OutProperties) const;

    void AddProperty(std::unique_ptr<FProperty> Property);
    const FProperty* FindProperty(const char* PropertyName) const;
    const TArray<std::unique_ptr<FProperty>>& GetProperties() const { return Properties; }

private:
    const char* Name = nullptr;
    UClass* SuperClass = nullptr;
    size_t ClassSize = 0;
    uint32 ClassFlags = 0;
    FCreateObjectFunc CreateFunc = nullptr;

    TArray<std::unique_ptr<FProperty>> Properties;
};
