#pragma once
class URenderer;
static unsigned int NextID = 0;

// 화면의 경계 위치를 나타내는 변수
const float leftBorder = -1.0f;
const float rightBorder = 1.0f;
const float topBorder = 1.0f;
const float bottomBorder = -1.0f;

class UGameObject
{
public:
    UGameObject() : ObjectID(NextID++)
    {
    }
    virtual void Update(float DeltaTime) = 0;
    virtual void Render(URenderer& Renderer) = 0;

    unsigned int GetID() const { return ObjectID; }

    bool IsActive() const { return bActive; }
    void SetActive(bool InActive) { bActive = InActive; }


protected:
    bool        bActive = true;
    const char* Tag = "None";
private:
    const unsigned int ObjectID;

};
