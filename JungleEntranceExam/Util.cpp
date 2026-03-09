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