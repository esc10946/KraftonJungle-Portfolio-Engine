#include "Animation/AnimationSequenceBase.h"

void UAnimationSequenceBase::AddNotify(const FAnimNotifyEvent& Notify)
{
    if (!Notify.NotifyName.IsValid())
    {
        return;
    }

    Notifies.push_back(Notify);
}

bool UAnimationSequenceBase::RemoveNotifyAt(int32 NotifyIndex)
{
    if (NotifyIndex < 0 || NotifyIndex >= static_cast<int32>(Notifies.size()))
    {
        return false;
    }

    Notifies.erase(Notifies.begin() + NotifyIndex);
    return true;
}
