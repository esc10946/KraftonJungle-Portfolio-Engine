#include "Class.h"
#include "Object/Object.h"

namespace
{
    size_t GetExpectedPropertySize(EReflectedPropertyType Type)
    {
        switch (Type)
        {
        case EReflectedPropertyType::Bool:
            return sizeof(bool);
        case EReflectedPropertyType::Int32:
            return sizeof(int32);
        case EReflectedPropertyType::Float:
            return sizeof(float);
        case EReflectedPropertyType::Name:
            return sizeof(FName);
        case EReflectedPropertyType::String:
            return sizeof(FString);
        case EReflectedPropertyType::Object:
            return sizeof(UObject*);
        case EReflectedPropertyType::StaticMeshAsset:
            return sizeof(FStaticMeshAssetRef);
        case EReflectedPropertyType::SkeletalMeshAsset:
            return sizeof(FSkeletalMeshAssetRef);
        case EReflectedPropertyType::TextureAsset:
            return sizeof(FTextureAssetRef);
        case EReflectedPropertyType::MaterialAsset:
            return sizeof(FMaterialAssetRef);
        case EReflectedPropertyType::AnimationSequenceAsset:
            return sizeof(FAnimationSequenceAssetRef);
        case EReflectedPropertyType::CurveAsset:
            return sizeof(FCurveAssetRef);
        case EReflectedPropertyType::SceneAsset:
            return sizeof(FSceneAssetRef);
        case EReflectedPropertyType::SoundAsset:
            return sizeof(FSoundAssetRef);
        }

        return 0;
    }
}

UClass::UClass(const char* InName, UClass* InSuperClass, size_t InClassSize, uint32 InClassFlags, FCreateObjectFunc InCreateFunc)
    : Name(InName), SuperClass(InSuperClass), ClassSize(InClassSize), ClassFlags(InClassFlags), CreateFunc(InCreateFunc)
{
}

bool UClass::IsChildOf(const UClass* Other) const
{
    if (!Other)
    {
        return false;
    }

    for (const UClass* Class = this; Class; Class = Class->SuperClass)
    {
        if (Class == Other)
        {
            return true;
        }
    }

    return false;
}

UObject* UClass::CreateObject() const
{
    return CreateFunc ? CreateFunc() : nullptr;
}

void UClass::GetAllProperties(TArray<const FProperty*>& OutProperties) const
{
    if (SuperClass)
    {
        SuperClass->GetAllProperties(OutProperties);
    }

    for (const FProperty& Property : Properties)
    {
        OutProperties.push_back(&Property);
    }
}

void UClass::AddProperty(const FProperty& Property)
{
    if (!Property.Name)
    {
        return;
    }

    for (const FProperty& ExistingProperty : Properties)
    {
        if (ExistingProperty.Name && std::strcmp(ExistingProperty.Name, Property.Name) == 0)
        {
            return;
        }
    }

    const size_t ExpectedSize = GetExpectedPropertySize(Property.Type);
    if (ExpectedSize != 0 && Property.Size != ExpectedSize)
    {
        return;
    }

    Properties.push_back(Property);
}

const FProperty* UClass::FindProperty(const char* PropertyName) const
{
    if (!PropertyName)
    {
        return nullptr;
    }

    for (const FProperty& Property : Properties)
    {
        if (Property.Name && std::strcmp(Property.Name, PropertyName) == 0)
        {
            return &Property;
        }
    }

    return SuperClass ? SuperClass->FindProperty(PropertyName) : nullptr;
}
