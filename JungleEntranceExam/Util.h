#pragma once

#include <random>

//int RandomIntInRange(int min, int max);
//float RandomFloatInRange(float min, float max);
//
//float GetRandomFloat(float min, float max);

template<typename T>
float GetRandomFloat(T min, T max)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<T> dis(min, max);

    return dis(gen);
}