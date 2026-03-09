#pragma once

static unsigned int NextID = 0;

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
