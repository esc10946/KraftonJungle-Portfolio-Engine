#pragma once
#include "UScene.h"

class UGameManager;
class UBall;

class UGameScene:
    public UScene
{
public:
    UGameScene();
    ~UGameScene();

    void Update(float delta) override;
    bool HaveBalls();

    // UScene을(를) 통해 상속됨
    void Init() override;
    void Release() override;

    void AddObject(UGameObject* Object) override;

private:
    UGameManager* gameManager = nullptr;

    std::vector<UBall*> ActiveBallList;
};