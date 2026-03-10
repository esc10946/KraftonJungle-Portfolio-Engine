#pragma once

static unsigned int NextID = 0;

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

    virtual ~UGameObject() = default;
    unsigned int GetID() const { return ObjectID; }

private:
    const unsigned int ObjectID;
};
