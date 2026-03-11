#include "ItemLibrary.h"

FItemDesc ItemLibrary::MakeRandomItem()
{
    int r = rand() % 100;

    // 멀티볼 아이템 (확률 : --%)
    if (r < 30)
        return MakeMultiBall(2);
    // 스코어 보너스 아이템 (확률 : --%)
    if (r < 40)
        return MakeScoreBonus(100);
    // 패들 길이 증가 아이템 (확률 : --%)
    if (r < 60)
        return MakePaddleExpand(1.2f);
    // 패들 길이 감소 아이템 (확률 : --%)
    if (r < 70)
        return MakePaddleShrink(0.8f);
    // 공 속도 증가 아이템 (확률 : --%)
    if (r < 90)
        return MakeBallSpeedUp(1.5f);
    // 공 속도 감소 아이템 (확률 : --%)
    return MakeBallSpeedDown(0.7f);
}

FItemDesc ItemLibrary::MakeMultiBall(int Count)
{
    FItemDesc Desc;
    Desc.Type = EItemType::MultiBall;
    Desc.ApplyType = EItemApplyType::Immediate;
    Desc.IntValue = Count;
    Desc.DebugName = "MultiBall";
    return Desc;
}

FItemDesc ItemLibrary::MakeScoreBonus(int Score)
{
    FItemDesc Desc;
    Desc.Type = EItemType::ScoreBonus;
    Desc.ApplyType = EItemApplyType::Immediate;
    Desc.IntValue = Score;
    Desc.DebugName = "ScoreBonus";
    return Desc;
}

FItemDesc ItemLibrary::MakePaddleExpand(float DeltaSize)
{
    FItemDesc Desc;
    Desc.Type = EItemType::PaddleExpand;
    Desc.ApplyType = EItemApplyType::Immediate;
    Desc.FloatValue = DeltaSize;
    Desc.DebugName = "PaddleExpand";
    return Desc;
}

FItemDesc ItemLibrary::MakePaddleShrink(float DeltaSize)
{
    FItemDesc Desc;
    Desc.Type = EItemType::PaddleShrink;
    Desc.ApplyType = EItemApplyType::Immediate;
    Desc.FloatValue = DeltaSize;
    Desc.DebugName = "PaddleShrink";
    return Desc;
}

FItemDesc ItemLibrary::MakeBallSpeedUp(float Multiplier)
{
    FItemDesc Desc;
    Desc.Type = EItemType::BallSpeedUp;
    Desc.ApplyType = EItemApplyType::Immediate;
    Desc.FloatValue = Multiplier;
    Desc.DebugName = "BallSpeedUp";
    return Desc;
}

FItemDesc ItemLibrary::MakeBallSpeedDown(float Multiplier)
{
    FItemDesc Desc;
    Desc.Type = EItemType::BallSpeedDown;
    Desc.ApplyType = EItemApplyType::Immediate;
    Desc.FloatValue = Multiplier;
    Desc.DebugName = "BallSpeedDown";
    return Desc;
}
