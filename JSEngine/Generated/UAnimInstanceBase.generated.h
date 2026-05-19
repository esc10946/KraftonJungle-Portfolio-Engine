#pragma once

class UAnimInstanceBase;

template<>
struct TIsUClassReflected<UAnimInstanceBase>
{
    static constexpr bool Value = true;
};
