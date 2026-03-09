#pragma once
#include "Types.h"
#include <vector>
#include <string>

class UGameObject;

class UScene
{
public:
    UScene() = default;
    virtual ~UScene() = default;

    UGameObject* SearchObject(unsigned int ID);
    virtual void Update(float delta);
    virtual void Render();

    void SetActive(bool doActive) { bisActive = doActive; }

    virtual void Init() = 0;
    virtual void Release() = 0;

    ESceneType GetSceneType() { return SceneType; }

private:
    virtual void AddObject(UGameObject* Object) { UGameObjectList.push_back(Object); }
    void RemoveObject(UGameObject* Object);


protected:
    //std::string SceneName;
    std::vector<UGameObject*> UGameObjectList;
    bool bisActive;
    ESceneType SceneType;
};