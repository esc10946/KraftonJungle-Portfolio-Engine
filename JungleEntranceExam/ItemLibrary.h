#pragma once

#include "UItem.h"

class ItemLibrary
{
public:
    static FItemDesc MakeRandomItem();

    static FItemDesc MakeMultiBall(int Count = 2);
    static FItemDesc MakeScoreBonus(int Score = 100);
    static FItemDesc MakePaddleExpand(float DeltaSize, float Duration);
    static FItemDesc MakePaddleShrink(float DeltaSize, float Duration);
    static FItemDesc MakeBallSpeedUp(float Multiplier, float Duration);
    static FItemDesc MakeBallSpeedDown(float Multiplier, float Duration);
    static FItemDesc MakePiercingBall(float Duration);
    static FItemDesc MakeLaserMode(float Duration);
};

