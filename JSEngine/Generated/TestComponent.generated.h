#pragma once

class TestComponent;

template<>
struct TIsUClassReflected<TestComponent>
{
    static constexpr bool Value = true;
};
