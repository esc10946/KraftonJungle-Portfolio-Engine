#pragma once
#include "EditorViewer.h"

class UAnimSingleNodeInstance;
class UAnimationSequence;

//필요없나?
struct ViewerState
{
	bool bIsPause;
	bool bIsLoop;

    uint32 CurrentFrame;
    uint32 TotalFrame;
	float CurrentTime;
	float Duration;

	bool bIsAnimationOver()
    {
        return CurrentFrame >= TotalFrame ||
               CurrentTime	>= Duration;
    }
};

class FAnimationViewer : public FEditorViewer
{
public:
    ~FAnimationViewer() override;

    void Shutdown() override;
    void ChangeTarget(const FString& InFileName) override;

    bool IsAnimationSequenceCompatible(const FString& AnimationPath) const;
    bool SetAnimationSequence(const FString& AnimationPath);
    void ClearAnimationSequence();
    void PlayAnimation();
    void PauseAnimation();
    void StopAnimation();

    void SetAnimationTime(float Time);
    void SetLooping(bool bInLooping);
    bool IsLooping() const;
    void SetPlayRate(float InPlayRate);
    float GetPlayRate() const;

    const FString& GetAnimationSequencePath() const { return AnimationSequencePath; }
    UAnimationSequence* GetAnimationSequence() const { return CurrentAnimationSequence; }
    float GetAnimationTime() const;
    float GetAnimationLength() const;
    bool IsAnimationPlaying() const;
    bool IsAnimationPaused() const;

private:
    UAnimSingleNodeInstance* PreviewAnimInstance = nullptr;
    UAnimationSequence* CurrentAnimationSequence = nullptr;
    FString AnimationSequencePath;
    float PlayRate = 1.0f;
    bool bLooping = true;
};
