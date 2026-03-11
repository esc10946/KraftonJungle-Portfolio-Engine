#pragma once
#include "URenderer.h"
static unsigned int NextID = 0;

// ?붾㈃??寃쎄퀎 ?꾩튂瑜??섑??대뒗 蹂??
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
    // 臾쇰━/?대룞 ?낅뜲?댄듃
    virtual void Update(float deltaTime) = 0;

    // ?뚮뜑留?
    virtual void Render(URenderer& renderer) = 0;


    unsigned int GetID() const { return ObjectID; }

private:
    const unsigned int ObjectID;
};
