#pragma once

#include "Animation/Notify/AnimNotifyState.h"

class UAnimInstance;

#include "Source/Engine/Animation/Notify/AnimNotifyState_ParryWindow.generated.h"

UCLASS()
class UAnimNotifyState_ParryWindow : public UAnimNotifyState
{
public:
	GENERATED_BODY()
	UAnimNotifyState_ParryWindow() = default;
	~UAnimNotifyState_ParryWindow() override = default;

	UPROPERTY(Edit, Save, Category="ParryWindow", DisplayName="Log State")
	bool bLogState = false;

	void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim, float TotalDuration) override;
	void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Anim) override;

	static bool IsParryWindowActive(UAnimInstance* AnimInstance);
	static bool ReportSuccessfulParry(UAnimInstance* AnimInstance);
	static bool ConsumeSuccessfulParry(UAnimInstance* AnimInstance);
};
