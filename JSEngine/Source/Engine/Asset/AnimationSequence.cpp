#include "Asset/AnimationSequence.h"

#include "Object/ObjectFactory.h"

DEFINE_CLASS(UAnimationSequence, UAnimationSequenceBase)
REGISTER_FACTORY(UAnimationSequence)

UAnimationSequence::~UAnimationSequence()
{
    delete SequenceData;
    SequenceData = nullptr;
}

void UAnimationSequence::SetSequenceData(FAnimationSequence* InSequenceData)
{
    if (SequenceData == InSequenceData)
    {
        return;
    }

    delete SequenceData;
    SequenceData = InSequenceData;
}

FAnimationSequence* UAnimationSequence::GetSequenceData()
{
    return SequenceData;
}

const FAnimationSequence* UAnimationSequence::GetSequenceData() const
{
    return SequenceData;
}

const FString& UAnimationSequence::GetAssetPathFileName() const
{
    return GetSourcePath();
}

const FString& UAnimationSequence::GetSourcePath() const
{
    static FString Empty = {};
    return SequenceData ? SequenceData->SourcePath : Empty;
}

bool UAnimationSequence::HasValidSequenceData() const
{
    return SequenceData != nullptr && (!SequenceData->BoneTracks.empty() || !SequenceData->ShapeKeyTracks.empty());
}

float UAnimationSequence::GetPlayLength() const
{
    return SequenceData ? SequenceData->DurationSeconds : 0.0f;
}

