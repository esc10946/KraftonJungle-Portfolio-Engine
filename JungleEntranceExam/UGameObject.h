#pragma once
class URenderer;
static unsigned int NextID = 0;

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

    //const char* GetTag() const { return Tag; }
    //void SetTag(const char* InTag) { Tag = InTag; }

protected:
    bool        bActive = true;
    const char* Tag = "None";
private:
    const unsigned int ObjectID;

};
