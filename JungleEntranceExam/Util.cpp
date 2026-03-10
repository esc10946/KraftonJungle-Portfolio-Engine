#include "Util.h"


// 범위 내 랜덤 int값 생성 ( [min, max] )
int RandomIntInRange(int min, int max)
{
    return min + rand() % (max - min + 1);
}

// 범위 내 랜덤 float값 생성 ( [min, max] )
float RandomFloatInRange(float min, float max)
{
    return min + (max - min) * ((float)rand() / (float)RAND_MAX);
}

float GetRandomFloat(float min, float max)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, max);

    return dis(gen);
}

