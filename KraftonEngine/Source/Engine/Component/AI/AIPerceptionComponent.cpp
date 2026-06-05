#include "Component/AI/AIPerceptionComponent.h"

#include "Component/AI/AIBlackboardComponent.h"
#include "Component/AI/EnemyAIBrainComponent.h"
#include "Component/AI/PhaseComponent.h"
#include "Component/Combat/CombatStateComponent.h"
#include "Component/Combat/CombatMoveComponent.h"
#include "Component/Combat/HealthComponent.h"
#include "GameFramework/AActor.h"

#include <cmath>

namespace
{
    const FName Key_HasTarget    = FName("HasTarget");
    const FName Key_CanSee       = FName("CanSee");
    const FName Key_Distance     = FName("Distance");
    const FName Key_Angle        = FName("Angle");
    const FName Key_AbsAngle     = FName("AbsAngle");
    const FName Key_SelfHealth   = FName("SelfHealth");
    const FName Key_SelfPosture  = FName("SelfPosture");
    const FName Key_TargetHealth = FName("TargetHealth");
    const FName Key_TargetPosture= FName("TargetPosture");
    const FName Key_TargetThreat = FName("TargetThreat");
    const FName Key_ThreatScore  = FName("ThreatScore");
    const FName Key_Phase        = FName("Phase");
    const FName Key_TargetPerilous   = FName("TargetPerilous");
    const FName Key_TargetInRecovery = FName("TargetInRecovery");
    const FName Key_TargetMovePhase  = FName("TargetMovePhase");
}

void UAIPerceptionComponent::RecordStimulus(EAISenseType Type, AActor* Source, const FVector& Location, float Strength)
{
    // 기존 자극 노화 + 만료 제거.
    for (auto It = Stimuli.begin(); It != Stimuli.end(); )
    {
        ++It->AgeTicks;
        if (It->AgeTicks > StimulusMemoryTicks)
        {
            It = Stimuli.erase(It);
        }
        else
        {
            ++It;
        }
    }

    FAISenseStimulus Stim;
    Stim.Type     = Type;
    Stim.Source   = Source;
    Stim.Location = Location;
    Stim.Strength = Strength;
    Stim.AgeTicks = 0;
    Stimuli.insert(Stimuli.begin(), Stim);

    // 디버거 가독성을 위해 최근 16개만 유지.
    while (static_cast<int32>(Stimuli.size()) > 16)
    {
        Stimuli.pop_back();
    }
}

void UAIPerceptionComponent::UpdateSenses()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }
    UAIBlackboardComponent* BB    = Owner->GetComponentByClass<UAIBlackboardComponent>();
    UEnemyAIBrainComponent* Brain = Owner->GetComponentByClass<UEnemyAIBrainComponent>();
    if (!BB || !Brain)
    {
        return;
    }

    // ── 자기 상태 ──
    float SelfHealth  = 1.0f;
    float SelfPosture = 1.0f;
    if (UHealthComponent* H = Owner->GetComponentByClass<UHealthComponent>())
    {
        SelfHealth = H->GetHealthRatio();
    }
    if (UCombatStateComponent* C = Owner->GetComponentByClass<UCombatStateComponent>())
    {
        SelfPosture = C->GetPoiseRatio();
    }
    BB->SetFloat(Key_SelfHealth, SelfHealth);
    BB->SetFloat(Key_SelfPosture, SelfPosture);

    int32 Phase = 1;
    if (UPhaseComponent* P = Owner->GetComponentByClass<UPhaseComponent>())
    {
        Phase = P->GetCurrentPhase();
    }
    BB->SetFloat(Key_Phase, static_cast<float>(Phase));

    // ── 타깃 ──
    AActor*    TargetActor = Brain->GetTarget();
    const bool bHasTarget  = Brain->HasValidTarget();
    BB->SetBool(Key_HasTarget, bHasTarget);
    BB->SetTarget(TargetActor);

    if (!bHasTarget || !TargetActor)
    {
        bCanSeeTarget = false;
        BB->SetBool(Key_CanSee, false);
        BB->SetFloat(Key_Distance, 9999.0f);
        BB->SetFloat(Key_ThreatScore, 0.0f);
        return;
    }

    const float Dist     = Brain->GetDistanceToTarget();
    const float Angle    = Brain->GetAngleToTarget();
    const float AbsAngle = std::fabs(Angle);
    BB->SetFloat(Key_Distance, Dist);
    BB->SetFloat(Key_Angle, Angle);
    BB->SetFloat(Key_AbsAngle, AbsAngle);

    float TargetHealth  = 1.0f;
    float TargetPosture = 1.0f;
    float TargetThreat  = 0.0f;
    if (UHealthComponent* TH = TargetActor->GetComponentByClass<UHealthComponent>())
    {
        TargetHealth = TH->GetHealthRatio();
    }
    float TargetPerilous = 0.0f;
    if (UCombatStateComponent* TC = TargetActor->GetComponentByClass<UCombatStateComponent>())
    {
        TargetPosture = TC->GetPoiseRatio();
        TargetThreat  = TC->GetAttackThreatRemaining();
        TargetPerilous = static_cast<float>(static_cast<int32>(TC->GetActivePerilousType()));
    }
    BB->SetFloat(Key_TargetHealth, TargetHealth);
    BB->SetFloat(Key_TargetPosture, TargetPosture);
    BB->SetFloat(Key_TargetThreat, TargetThreat);

    // ── 프레임 데이터: 타깃이 후딜 중이면 punish 기회 (Phase 2) ──
    bool  TargetInRecovery = false;
    float TargetMovePhase  = 0.0f;
    if (UCombatMoveComponent* TM = TargetActor->GetComponentByClass<UCombatMoveComponent>())
    {
        TargetInRecovery = TM->IsInRecovery();
        TargetMovePhase  = static_cast<float>(TM->GetPhaseInt());
    }
    BB->SetFloat(Key_TargetPerilous, TargetPerilous);
    BB->SetBool(Key_TargetInRecovery, TargetInRecovery);
    BB->SetFloat(Key_TargetMovePhase, TargetMovePhase);

    // ── 시야 콘: 거리 + FOV 안쪽 ──
    const bool bInRange = Dist <= SightRange;
    const bool bInFov   = AbsAngle <= (FieldOfViewDegrees * 0.5f);
    const bool bVisible = bInRange && bInFov;
    bCanSeeTarget       = bVisible;
    BB->SetBool(Key_CanSee, bVisible);

    // ── 위협 점수: 타깃이 공격을 커밋했고 사정권이면 1 ──
    const float ThreatScore = (TargetThreat > 0.0f && Dist <= SightRange) ? 1.0f : 0.0f;
    BB->SetFloat(Key_ThreatScore, ThreatScore);

    if (bVisible)
    {
        const FVector TargetLoc = TargetActor->GetActorLocation();
        BB->SetLastSeenLocation(TargetLoc);
        RecordStimulus(EAISenseType::Sight, TargetActor, TargetLoc, 1.0f);
    }
    if (Dist <= ProximityRange)
    {
        RecordStimulus(EAISenseType::Proximity, TargetActor, TargetActor->GetActorLocation(), 1.0f);
    }
}
