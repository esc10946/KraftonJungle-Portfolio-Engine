#pragma once

static unsigned int NextID = 0;

// 화면의 경계 위치를 나타내는 변수
inline const float leftBorder = -1.0f;
inline const float rightBorder = 1.0f;
inline const float topBorder = 1.0f;
inline const float bottomBorder = -1.0f;

class UGameObject
{
public:
    UGameObject() : ObjectID(NextID++)
    {
    }

    virtual ~UGameObject() = default;
    unsigned int GetID() const { return ObjectID; }

private:
    const unsigned int ObjectID;
};
