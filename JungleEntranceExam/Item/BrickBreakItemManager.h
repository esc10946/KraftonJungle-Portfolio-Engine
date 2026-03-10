#pragma once

//#include "BrickBreakItemTypes.h"
#include "../UItem.h"
#include "ItemEffectReceiver.h"

class UItem;

class UBrickBreakItemManager
{
public:
    UBrickBreakItemManager();
    ~UBrickBreakItemManager();

public:
    void Initialize();
    void Shutdown();

    void Update(float DeltaTime);
    void Render(URenderer& renderer);
    void Clear();

public:
    void SpawnItem(const FItemDesc& ItemDesc, const FVector& SpawnPosition, const FVector& Direction);
    //void SpawnItem(const FItemSpawnInfo& SpawnInfo);

    //void CheckPlayerCollision(const FRect& PaddleBounds, IItemEffectReceiver* Receiver);
    void RemoveDeadItems();

    int GetItemCount() const;

private:
    void Reserve(int NewCapacity);
    void AddItem(UItem* NewItem);
    void RemoveItemAt(int Index);

private:
    UItem** ItemList = nullptr;
    int ItemCount = 0;
    int Capacity = 0;
};