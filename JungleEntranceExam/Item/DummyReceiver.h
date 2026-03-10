#pragma once

#include "ItemEffectReceiver.h"
#include <windows.h>

class DummyReceiver : public IItemEffectReceiver
{
public:
    virtual void AddScore(int Amount) override
    {
        OutputDebugStringA("AddScore called\n");
    }

    virtual void SpawnExtraBalls(int Count) override
    {
        OutputDebugStringA("SpawnExtraBalls called\n");
    }

    virtual void ModifyPaddleSize(float DeltaSize, float Duration) override
    {
        OutputDebugStringA("ModifyPaddleSize called\n");
    }

    virtual void ModifyBallSpeed(float Multiplier, float Duration) override
    {
        OutputDebugStringA("ModifyBallSpeed called\n");
    }
};