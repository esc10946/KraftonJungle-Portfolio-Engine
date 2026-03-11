#pragma once
#include "UScene.h"

class UUIButton;
class UGameObject;

class UTitleScene :
    public UScene
{

public:
    UTitleScene() = default;
    virtual ~UTitleScene() { Release(); }

    // UScene??瑜? ?듯빐 ?곸냽??
    void Init() override;
    void Update(float delta) override;
    void Render(URenderer render) override;
    void Release() override;

    void GameStart();
    void GameEnd();

private:
    UGameObject* TitleLogo = nullptr;
    UUIButton* StartButton = nullptr;
    UUIButton* EndButton = nullptr;
    UUIButton* CreditButton = nullptr;

    // UScene??瑜? ?듯빐 ?곸냽??
    void UIRender() override;
};

