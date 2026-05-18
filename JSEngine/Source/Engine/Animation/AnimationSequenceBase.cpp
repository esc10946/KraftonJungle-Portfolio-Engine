#include "Animation/AnimationSequenceBase.h"

void UAnimationSequenceBase::AddNotify(const FAnimNotifyEvent& Notify)
{
    if (!Notify.NotifyName.IsValid())
    {
        return;
    }

    Notifies.push_back(Notify);
}
