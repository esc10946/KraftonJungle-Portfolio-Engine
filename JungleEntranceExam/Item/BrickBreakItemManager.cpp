#include "BrickBreakItemManager.h"
//#include "BrickBreakItem.h"
//#include "BrickBreakItemLibrary.h"

UBrickBreakItemManager::UBrickBreakItemManager()
{
}

UBrickBreakItemManager::~UBrickBreakItemManager()
{
    Shutdown();
}

void UBrickBreakItemManager::Initialize()
{
}

void UBrickBreakItemManager::Shutdown()
{
    Clear();
}

void UBrickBreakItemManager::Update(float DeltaTime)
{
    for (int i = 0; i < ItemCount; ++i)
    {
        if (ItemList[i] != nullptr)
        {
            ItemList[i]->Update(DeltaTime);
        }
    }

    RemoveDeadItems();
}

void UBrickBreakItemManager::Render(URenderer& renderer)
{
    for (int i = 0; i < ItemCount; ++i)
    {
        if (ItemList[i] != nullptr)
        {
            ItemList[i]->Render(renderer);
        }
    }
}

void UBrickBreakItemManager::Clear()
{
    for (int i = 0; i < ItemCount; ++i)
    {
        delete ItemList[i];
        ItemList[i] = nullptr;
    }

    delete[] ItemList;
    ItemList = nullptr;

    ItemCount = 0;
    Capacity = 0;
}

void UBrickBreakItemManager::SpawnItem(const FItemDesc& ItemDesc, const FVector& SpawnPosition, const FVector& Direction)
{
    UItem* NewItem = new UItem(SpawnPosition, Direction, 0.1f, 0.05f, ItemDesc);
    AddItem(NewItem);
}

//void UBrickBreakItemManager::SpawnItem(const FItemSpawnInfo& SpawnInfo)
//{
//    FItemDesc Desc;
//
//    switch (SpawnInfo.Type)
//    {
//    case EItemType::MultiBall:
//        Desc = UBrickBreakItemLibrary::MakeMultiBall();
//        break;
//
//    case EItemType::ScoreBonus:
//        Desc = UBrickBreakItemLibrary::MakeScoreBonus();
//        break;
//
//    default:
//        return;
//    }
//
//    ABrickBreakItem* NewItem = new ABrickBreakItem();
//    NewItem->Initialize(Desc, SpawnInfo);
//    AddItem(NewItem);
//}

//void UBrickBreakItemManager::CheckPlayerCollision(const FRect& PaddleBounds, IItemEffectReceiver* Receiver)
//{
//    for (int i = 0; i < ItemCount; ++i)
//    {
//        UItem* Item = ItemList[i];
//        if (Item == nullptr || !Item->IsAlive())
//            continue;
//
//        /*if (Item->CheckCollision(PaddleBounds))
//        {
//            Item->ApplyTo(Receiver);
//        }*/
//    }
//}

void UBrickBreakItemManager::RemoveDeadItems()
{
    for (int i = ItemCount - 1; i >= 0; --i)
    {
        if (ItemList[i] == nullptr || !ItemList[i]->IsAlive())
        {
            RemoveItemAt(i);
        }
    }
}

int UBrickBreakItemManager::GetItemCount() const
{
    return ItemCount;
}

void UBrickBreakItemManager::Reserve(int NewCapacity)
{
    if (NewCapacity <= Capacity)
        return;

    UItem** NewList = new UItem * [NewCapacity];

    for (int i = 0; i < ItemCount; ++i)
    {
        NewList[i] = ItemList[i];
    }

    for (int i = ItemCount; i < NewCapacity; ++i)
    {
        NewList[i] = nullptr;
    }

    delete[] ItemList;
    ItemList = NewList;
    Capacity = NewCapacity;
}

void UBrickBreakItemManager::AddItem(UItem* NewItem)
{
    if (ItemCount >= Capacity)
    {
        const int NewCapacity = (Capacity == 0) ? 4 : Capacity * 2;
        Reserve(NewCapacity);
    }

    ItemList[ItemCount] = NewItem;
    ++ItemCount;
}

void UBrickBreakItemManager::RemoveItemAt(int Index)
{
    if (Index < 0 || Index >= ItemCount)
        return;

    delete ItemList[Index];
    ItemList[Index] = nullptr;

    for (int i = Index; i < ItemCount - 1; ++i)
    {
        ItemList[i] = ItemList[i + 1];
    }

    ItemList[ItemCount - 1] = nullptr;
    --ItemCount;
}