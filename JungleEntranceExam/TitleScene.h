#pragma once
#include "UScene.h"

class UTitleScene :
    public UScene
{

public:
    UTitleScene(){
        SceneName = "TitleScene";
        Init();
    }

    ~UTitleScene() = default;

    void GameStart();
    void GameEnd();
    void Credit();

    // UScene을(를) 통해 상속됨
    void Init() override;
    void Release() override;
};

