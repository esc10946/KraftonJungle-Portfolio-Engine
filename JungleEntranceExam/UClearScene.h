#pragma once
#include "UScene.h"

class UClearScene :
    public UScene
{ 
public:
    UClearScene() = default;
    ~UClearScene() = default;

    // UScene??瑜? ?듯빐 ?곸냽??
    void Render(URenderer renderer) override;
    void UIRender() override;
    void Init() override;
    void Release() override;
    static int FinalScore;
};

