#pragma once
#include "UScene.h"

class UGameManager;
class UBall;
class UBar;
class UBlock;

class UGameScene:
    public UScene
{
public:
    UGameScene();
    ~UGameScene();

    void UIRender();

    // UScene을(를) 통해 상속됨
    void Update(float delta) override;
    void Render(URenderer render) override;
    bool HaveBalls();

    // UScene을(를) 통해 상속됨
    void Init() override;
    void Release() override;

    void AddObject(UGameObject* Object) override;
    bool bIsBrickEmpty();

    std::vector<UBall*>& GetActiveBalls();
    void AddBall(UBall* ball);

private:
    UGameManager* gameManager = nullptr;

    std::vector<UBlock*> stageblocks;
    std::vector<UBall*> ActiveBallList;
    UBar* Bar_1;
    UBar* Bar_2;
};