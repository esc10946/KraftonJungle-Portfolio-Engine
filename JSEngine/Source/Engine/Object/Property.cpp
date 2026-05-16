#include "Property.h"
#include "Class.h"
#include "Object.h"

void FProperty::SerializeItem(FArchive& Ar, UObject* Container) const
{
    if (HasPropertyFlag(Flags, EPropertyFlags::Transient))
        return;

    // 프로퍼티 이름을 키로 먼저 세팅
    Ar.SetCurrentKey(FString(Name));

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
    case EReflectedPropertyType::Name:
        Ar << *ContainerPtrToValuePtr<FName>(Container);
        break;
    case EReflectedPropertyType::Object:
        // Phase 5 — GUID/경로 직렬화로 처리 예정
        break;
    }
}
