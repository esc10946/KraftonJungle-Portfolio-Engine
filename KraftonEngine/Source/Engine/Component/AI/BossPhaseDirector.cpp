#include "Component/AI/BossPhaseDirector.h"

#include "Component/AI/AIBlackboardComponent.h"
#include "Component/AI/PhaseComponent.h"
#include "Component/Combat/CombatStateComponent.h"
#include "GameFramework/AActor.h"
#include "GameFramework/World.h"

#include <algorithm>

namespace
{
    const FName Key_PhaseAggression = FName("PhaseAggression");
    const FName Key_PhaseEntry      = FName("PhaseEntry");

    float GameTimeSeconds(const UActorComponent* Component)
    {
        AActor* OwnerActor = Component ? Component->GetOwner() : nullptr;
        UWorld* World      = OwnerActor ? OwnerActor->GetWorld() : nullptr;
        return World ? World->GetGameTimeSeconds() : 0.0f;
    }
}

void UBossPhaseDirector::BeginPlay()
{
    UActorComponent::BeginPlay();
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }
    if (UPhaseComponent* Phase = Owner->GetComponentByClass<UPhaseComponent>())
    {
        Phase->OnPhaseChanged.AddUObject(this, &UBossPhaseDirector::HandlePhaseChanged);
        ApplyPhase(Phase->GetCurrentPhase(), false);
    }
    else
    {
        ApplyPhase(1, false);
    }
}

void UBossPhaseDirector::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction)
{
    UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 페이즈 전환 슈퍼아머 윈도우 만료 처리.
    if (bSuperArmorEngaged && !IsInPhaseTransition())
    {
        bSuperArmorEngaged = false;
        if (AActor* Owner = GetOwner())
        {
            if (UCombatStateComponent* Combat = Owner->GetComponentByClass<UCombatStateComponent>())
            {
                Combat->SetSuperArmor(false);
            }
            if (UAIBlackboardComponent* BB = Owner->GetComponentByClass<UAIBlackboardComponent>())
            {
                BB->SetBool(Key_PhaseEntry, false);
            }
        }
    }
}

float UBossPhaseDirector::GetPhaseAggression(int32 Phase) const
{
    const float Value = BaseAggression + AggressionPerPhase * static_cast<float>((std::max)(0, Phase - 1));
    return (std::max)(0.0f, Value);
}

bool UBossPhaseDirector::IsInPhaseTransition() const
{
    return GameTimeSeconds(this) < TransitionUntilSeconds;
}

void UBossPhaseDirector::HandlePhaseChanged(UPhaseComponent* /*Component*/, int32 /*OldPhase*/, int32 NewPhase)
{
    ApplyPhase(NewPhase, true);
    OnPhaseTransition.Broadcast(this, NewPhase);
}

void UBossPhaseDirector::ApplyPhase(int32 Phase, bool bIsTransition)
{
    TrackedPhase = Phase;
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    UCombatStateComponent*  Combat = Owner->GetComponentByClass<UCombatStateComponent>();
    UAIBlackboardComponent* BB     = Owner->GetComponentByClass<UAIBlackboardComponent>();

    if (BB)
    {
        BB->SetFloat(Key_PhaseAggression, GetPhaseAggression(Phase));
    }

    if (bIsTransition)
    {
        if (Combat)
        {
            if (bResetPostureOnPhaseChange)
            {
                Combat->ResetPoise();
            }
            if (PhaseTransitionSuperArmorSeconds > 0.0f)
            {
                Combat->SetSuperArmor(true);
                bSuperArmorEngaged     = true;
                TransitionUntilSeconds = GameTimeSeconds(this) + PhaseTransitionSuperArmorSeconds;
            }
        }
        if (BB)
        {
            BB->SetBool(Key_PhaseEntry, true);
        }
    }
}
