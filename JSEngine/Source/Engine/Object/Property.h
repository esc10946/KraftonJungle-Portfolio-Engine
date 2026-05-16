#pragma once
#include "Object/FName.h"
#include "Object/ObjectMacros.h"
#include "Serialization/Archive.h"
#include <type_traits>

class UObject;
class UClass;
enum class EReflectedPropertyType : uint8
{
    Bool,
    Int32,
    Float,
    Name,
    String,
    Object,
    Asset,
};

template<typename T> 
struct TReflectedPropertyType; // 선언만 있으면 링크 에러

// 지원 타입마다 이게 다 있어야 합니다
template<> struct TReflectedPropertyType<float>    { static constexpr EReflectedPropertyType Value = EReflectedPropertyType::Float; };
template<> struct TReflectedPropertyType<bool>     { static constexpr EReflectedPropertyType Value = EReflectedPropertyType::Bool; };
template<> struct TReflectedPropertyType<int32>    { static constexpr EReflectedPropertyType Value = EReflectedPropertyType::Int32; };
template<> struct TReflectedPropertyType<FName>    { static constexpr EReflectedPropertyType Value = EReflectedPropertyType::Name; };
template<> struct TReflectedPropertyType<FString>  { static constexpr EReflectedPropertyType Value = EReflectedPropertyType::String; };
template<> struct TReflectedPropertyType<UObject*> { static constexpr EReflectedPropertyType Value = EReflectedPropertyType::Object; };

// EPropertyFlags 규칙
// Edit      -> Read | Write | Edit
// Read      -> Read
// Write     -> Write
// Transient -> Transient
// LuaRead   -> LuaRead
// LuaWrite  -> LuaWrite
//
// 예시) UPROPERTY(Edit, LuaRead)
// -> Read | Write | Edit | LuaRead
//
// 예시) UPROPERTY(Edit, LuaRead, LuaWrite)
// -> Read | Write | Edit | LuaRead | LuaWrite

enum class EPropertyFlags : uint32
{
    None = 0,
    Read = 1 << 0,
    Write = 1 << 1,
    Edit = 1 << 2,
    Transient = 1 << 3,
    LuaRead = 1 << 4,
    LuaWrite = 1 << 5,
};

inline EPropertyFlags operator|(EPropertyFlags Lhs, EPropertyFlags Rhs)
{
    return static_cast<EPropertyFlags>(
        static_cast<uint32>(Lhs) | static_cast<uint32>(Rhs));
}

inline bool HasPropertyFlag(EPropertyFlags Value, EPropertyFlags Flag)
{
    return (static_cast<uint32>(Value) & static_cast<uint32>(Flag)) != 0;
}

struct FProperty
{
    const char* Name = nullptr;
    EReflectedPropertyType Type = EReflectedPropertyType::Float;
    size_t Offset = 0;
    size_t Size = 0;
    EPropertyFlags Flags = EPropertyFlags::None;
    UClass* ObjectClass = nullptr;

    void SerializeItem(FArchive& Ar, UObject* Container) const;
    UClass* GetObjectClass() const { return ObjectClass; }

    template <typename ValueType>
    ValueType* ContainerPtrToValuePtr(void* Container) const
    {
        if (!Container)
        {
            return nullptr;
        }

        return reinterpret_cast<ValueType*>(
            reinterpret_cast<uint8*>(Container) + Offset);
    }

    template <typename ValueType>
    const ValueType* ContainerPtrToValuePtr(const void* Container) const
    {
        if (!Container)
        {
            return nullptr;
        }

        return reinterpret_cast<const ValueType*>(
            reinterpret_cast<const uint8*>(Container) + Offset);
    }

    template <typename ValueType>
    bool IsCompatibleValueType() const
    {
        if constexpr (std::is_pointer_v<ValueType> &&
                      std::is_base_of_v<UObject, std::remove_pointer_t<ValueType>>)
        {
            return Type == EReflectedPropertyType::Object
                && Size == sizeof(UObject*);
        }
        else if constexpr (std::is_same_v<ValueType, FString>)
        {
            return (Type == EReflectedPropertyType::String ||
                    Type == EReflectedPropertyType::Asset)
                && Size == sizeof(FString);
        }
        else
        {
            return Type == TReflectedPropertyType<ValueType>::Value
                && Size == sizeof(ValueType);
        }
    }

    template <typename ValueType>
    bool GetPropertyValue_InContainer(const void* Container, ValueType& OutValue) const
    {
        if (!Container)
        {
            return false;
        }

        if (!IsCompatibleValueType<ValueType>())
        {
            return false;
        }

        if (!HasPropertyFlag(Flags, EPropertyFlags::Read))
        {
            return false;
        }

        const ValueType* ValuePtr = ContainerPtrToValuePtr<ValueType>(Container);
        if (!ValuePtr)
        {
            return false;
        }

        OutValue = *ValuePtr;
        return true;
    }

    template <typename ValueType>
    bool SetPropertyValue_InContainer(void* Container, const ValueType& InValue) const
    {
        if (!Container)
        {
            return false;
        }

        if (!IsCompatibleValueType<ValueType>())
        {
            return false;
        }

        if (!HasPropertyFlag(Flags, EPropertyFlags::Write))
        {
            return false;
        }

        ValueType* ValuePtr = ContainerPtrToValuePtr<ValueType>(Container);
        if (!ValuePtr)
        {
            return false;
        }

        *ValuePtr = InValue;
        return true;
    }
};
