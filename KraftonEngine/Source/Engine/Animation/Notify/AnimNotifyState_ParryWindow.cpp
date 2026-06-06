#include "AnimNotifyState_ParryWindow.h"

#include "Animation/AnimInstance.h"
#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Core/Logging/Log.h"
#include "Object/Ptr/WeakObjectPtr.h"

namespace
{
	TSet<TWeakObjectPtr<USkeletalMeshComponent>> GActiveParryMeshes;
	TSet<TWeakObjectPtr<USkeletalMeshComponent>> GSuccessfulParryMeshes;

	void PurgeInvalidParryMeshes()
	{
		for (auto It = GActiveParryMeshes.begin(); It != GActiveParryMeshes.end(); )
		{
			if (!IsValid(*It)) It = GActiveParryMeshes.erase(It);
			else ++It;
		}

		for (auto It = GSuccessfulParryMeshes.begin(); It != GSuccessfulParryMeshes.end(); )
		{
			if (!IsValid(*It)) It = GSuccessfulParryMeshes.erase(It);
			else ++It;
		}
	}

	USkeletalMeshComponent* GetMeshFromAnimInstance(UAnimInstance* AnimInstance)
	{
		if (!IsValid(AnimInstance))
		{
			return nullptr;
		}

		USkeletalMeshComponent* MeshComp = AnimInstance->GetOwningComponent();
		return IsValid(MeshComp) ? MeshComp : nullptr;
	}
}

void UAnimNotifyState_ParryWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* /*Anim*/, float /*TotalDuration*/)
{
	PurgeInvalidParryMeshes();
	if (!IsValid(MeshComp))
	{
		return;
	}

	GActiveParryMeshes.insert(TWeakObjectPtr<USkeletalMeshComponent>(MeshComp));

	if (bLogState)
	{
		UE_LOG("[AnimNotifyState_ParryWindow] BEGIN Mesh=%s", MeshComp->GetName().c_str());
	}
}

void UAnimNotifyState_ParryWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* /*Anim*/)
{
	PurgeInvalidParryMeshes();
	if (!IsValid(MeshComp))
	{
		return;
	}

	GActiveParryMeshes.erase(TWeakObjectPtr<USkeletalMeshComponent>(MeshComp));

	if (bLogState)
	{
		UE_LOG("[AnimNotifyState_ParryWindow] END Mesh=%s", MeshComp->GetName().c_str());
	}
}

bool UAnimNotifyState_ParryWindow::IsParryWindowActive(UAnimInstance* AnimInstance)
{
	PurgeInvalidParryMeshes();

	USkeletalMeshComponent* MeshComp = GetMeshFromAnimInstance(AnimInstance);
	if (!MeshComp)
	{
		return false;
	}

	return GActiveParryMeshes.find(TWeakObjectPtr<USkeletalMeshComponent>(MeshComp)) != GActiveParryMeshes.end();
}

bool UAnimNotifyState_ParryWindow::ReportSuccessfulParry(UAnimInstance* AnimInstance)
{
	PurgeInvalidParryMeshes();

	USkeletalMeshComponent* MeshComp = GetMeshFromAnimInstance(AnimInstance);
	if (!MeshComp)
	{
		return false;
	}

	const TWeakObjectPtr<USkeletalMeshComponent> Key(MeshComp);
	if (GActiveParryMeshes.find(Key) == GActiveParryMeshes.end())
	{
		return false;
	}

	GSuccessfulParryMeshes.insert(Key);
	return true;
}

bool UAnimNotifyState_ParryWindow::ConsumeSuccessfulParry(UAnimInstance* AnimInstance)
{
	PurgeInvalidParryMeshes();

	USkeletalMeshComponent* MeshComp = GetMeshFromAnimInstance(AnimInstance);
	if (!MeshComp)
	{
		return false;
	}

	const TWeakObjectPtr<USkeletalMeshComponent> Key(MeshComp);
	auto It = GSuccessfulParryMeshes.find(Key);
	if (It == GSuccessfulParryMeshes.end())
	{
		return false;
	}

	GSuccessfulParryMeshes.erase(It);
	return true;
}
