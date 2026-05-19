#pragma once

class UStateMachineAnimInstance;

template<>
struct TIsUClassReflected<UStateMachineAnimInstance>
{
    static constexpr bool Value = true;
};
