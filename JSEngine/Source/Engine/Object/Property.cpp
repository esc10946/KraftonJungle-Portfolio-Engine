#include "Property.h"
#include "Class.h"
#include "Object.h"

void FProperty::SerializeItem(FArchive& Ar, UObject* Container) const
{
    if (HasPropertyFlag(Flags, EPropertyFlags::Transient))
        return;

    const FString PropertyKey(Name);
    if (Ar.IsLoading() && !Ar.HasKey(PropertyKey))
    {
        return;
    }

    Ar.SetCurrentKey(PropertyKey);

    switch (Type)
    {
    case EReflectedPropertyType::Float:
        Ar << *ContainerPtrToValuePtr<float>(Container);
        break;
    case EReflectedPropertyType::Bool:
        Ar << *ContainerPtrToValuePtr<bool>(Container);
        break;
    case EReflectedPropertyType::Int32:
        Ar << *ContainerPtrToValuePtr<int32>(Container);
        break;
    case EReflectedPropertyType::String:
        Ar << *ContainerPtrToValuePtr<FString>(Container);
        break;
    case EReflectedPropertyType::Asset:
        Ar << *ContainerPtrToValuePtr<FString>(Container);
        break;
    case EReflectedPropertyType::Name:
        Ar << *ContainerPtrToValuePtr<FName>(Container);
        break;
    case EReflectedPropertyType::Object:
    {
        UObject** ObjectPtr = ContainerPtrToValuePtr<UObject*>(Container);
        if (!ObjectPtr)
        {
            break;
        }

        uint32 ObjectUUID = (Ar.IsSaving() && *ObjectPtr) ? (*ObjectPtr)->GetUUID() : 0;
        Ar << ObjectUUID;

        if (Ar.IsLoading())
        {
            *ObjectPtr = nullptr;
            UObjectManager::Get().RegisterPendingObjectReference(ObjectPtr, ObjectUUID, ObjectClass);
        }
        break;
    }
    }
}
