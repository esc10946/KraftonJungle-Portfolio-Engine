#pragma once
#include "UScene.h"

class UClearScene :
    public UScene
{ 
public:
    UClearScene() = default;
    ~UClearScene() = default;

    // UScene을(를) 통해 상속됨
    void Render(URenderer renderer) override;
    void UIRender() override;
    void Init() override;
    void Release() override;
    static int FinalScore;
};

