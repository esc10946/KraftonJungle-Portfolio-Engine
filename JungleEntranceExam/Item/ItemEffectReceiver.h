#pragma once

class IItemEffectReceiver
{
public:
    virtual ~IItemEffectReceiver() {}

public:
    virtual void SpawnExtraBalls(int Count) = 0;
    virtual void AddScore(int Amount) = 0;

    virtual void ModifyPaddleSize(float DeltaSize, float Duration) = 0;
    virtual void ModifyBallSpeed(float Multiplier, float Duration) = 0;
};