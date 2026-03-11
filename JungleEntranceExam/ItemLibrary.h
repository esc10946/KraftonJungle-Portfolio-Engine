#pragma once

#include "UItem.h"

class ItemLibrary
{
public:
    static FItemDesc MakeRandomItem();

    static FItemDesc MakeMultiBall(int Count = 2);
    static FItemDesc MakeScoreBonus(int Score = 100);
    static FItemDesc MakePaddleExpand(float DeltaSize);
    static FItemDesc MakePaddleShrink(float DeltaSize);
    static FItemDesc MakeBallSpeedUp(float Multiplier);
    static FItemDesc MakeBallSpeedDown(float Multiplier);
};

