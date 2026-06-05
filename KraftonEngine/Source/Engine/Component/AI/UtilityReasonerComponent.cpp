#include "Component/AI/UtilityReasonerComponent.h"

#include "Component/AI/AIBlackboardComponent.h"
#include "Component/AI/EnemyAttackComponent.h"
#include "GameFramework/AActor.h"
#include "Math/MathUtils.h"

#include <algorithm>
#include <cmath>

namespace
{
    const FName Key_Distance      = FName("Distance");
    const FName Key_AbsAngle      = FName("AbsAngle");
    const FName Key_Phase         = FName("Phase");
    const FName Key_TargetThreat  = FName("TargetThreat");
    const FName Key_TargetPosture = FName("TargetPosture");

    // 거리 곡선: 공격 사정대(MinRange~MaxRange) 중앙에서 1, 가장자리에서 0.4.
    float RangeCurve(const FEnemyAttackData& Attack, float Distance)
    {
        const float Span = (std::max)(1.0f, Attack.MaxRange - Attack.MinRange);
        const float T    = FMath::Clamp((Distance - Attack.MinRange) / Span, 0.0f, 1.0f);
        const float Off  = std::fabs(T - 0.5f) * 2.0f; // 0(중앙)~1(가장자리)
        return 1.0f - 0.6f * Off;
    }

    // 각도 곡선: 정면(0도)에서 1, 허용 각도 한계에서 0.3.
    float AngleCurve(const FEnemyAttackData& Attack, float AbsAngle)
    {
        const float Limit = (std::max)(1.0f, Attack.MaxAbsAngle);
        const float T     = FMath::Clamp(AbsAngle / Limit, 0.0f, 1.0f);
        return 1.0f - 0.7f * T;
    }
}

FName UUtilityReasonerComponent::GetChosenAction() const
{
    if (ChosenIndex >= 0 && ChosenIndex < static_cast<int32>(Candidates.size()))
    {
        return Candidates[ChosenIndex].ActionId;
    }
    return FName::None;
}

FName UUtilityReasonerComponent::SelectBestAction()
{
    Candidates.clear();
    ChosenIndex = -1;

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return FName::None;
    }
    UEnemyAttackComponent* AttackComp = Owner->GetComponentByClass<UEnemyAttackComponent>();
    UAIBlackboardComponent* BB        = Owner->GetComponentByClass<UAIBlackboardComponent>();
    if (!AttackComp || !BB)
    {
        return FName::None;
    }

    const float Distance      = BB->GetFloat(Key_Distance);
    const float AbsAngle      = BB->GetFloat(Key_AbsAngle);
    const int32 Phase         = static_cast<int32>(BB->GetFloat(Key_Phase) + 0.5f);
    const float TargetThreat  = BB->GetFloat(Key_TargetThreat);
    const float TargetPosture = BB->GetFloat(Key_TargetPosture);

    // 콤보 게이트용 마지막 사용 기술.
    const TArray<FName>& RecentMoves = BB->GetRecentMoves();
    const FName LastMove = RecentMoves.empty() ? FName::None : RecentMoves.front();

    float BestFinal = -1.0f;

    for (const FEnemyAttackData& Attack : AttackComp->Attacks)
    {
        if (!AttackComp->CanUseAttack(Attack, Phase, Distance, AbsAngle))
        {
            continue;
        }
        // 콤보 게이트: 선행 기술 요구 시 직전 기술이 일치해야 함.
        if (Attack.bRequiresPreviousAttack &&
            (!Attack.RequiredPreviousAttack.IsValid() || LastMove != Attack.RequiredPreviousAttack))
        {
            continue;
        }

        FUtilityScoreBreakdown B;
        B.Base  = (std::max)(0.0001f, Attack.Weight) * (Attack.Priority > 0.0f ? Attack.Priority : 1.0f);
        B.Range = RangeCurve(Attack, Distance);
        B.Angle = AngleCurve(Attack, AbsAngle);

        // 위협 곡선: 타깃이 공격을 커밋하면 Punish/Retreat 가치 상승.
        B.Threat = 1.0f;
        if (TargetThreat > 0.0f)
        {
            if (Attack.Tactic == EEnemyAttackTactic::Punish)
            {
                B.Threat = 1.5f;
            }
            else if (Attack.Tactic == EEnemyAttackTactic::Retreat)
            {
                B.Threat = 1.2f;
            }
        }

        // 체간 곡선: 타깃 체간이 낮으면(붕괴 임박) 압박/콤보 가치 상승.
        B.Posture = 1.0f;
        if (TargetPosture < 0.4f &&
            (Attack.Tactic == EEnemyAttackTactic::Pressure || Attack.Tactic == EEnemyAttackTactic::Combo))
        {
            B.Posture = 1.4f;
        }

        B.Phase = 1.0f; // MinPhase/MaxPhase 는 CanUseAttack 에서 이미 게이트.

        // 반복 패널티: 최근 같은 기술을 쓸수록 가치 하강.
        const int32 RepeatCount = BB->GetRecentMoveRepeat(Attack.AttackName);
        const float RepeatScale = FMath::Clamp(Attack.RepeatWeightScale, 0.0f, 1.0f);
        B.Repetition = std::pow(RepeatScale, static_cast<float>(RepeatCount));

        B.Final = B.Base * B.Range * B.Angle * B.Threat * B.Posture * B.Phase * B.Repetition;

        FUtilityCandidate Candidate;
        Candidate.ActionId  = Attack.AttackName;
        Candidate.Breakdown = B;
        Candidates.push_back(Candidate);

        if (B.Final > BestFinal)
        {
            BestFinal   = B.Final;
            ChosenIndex = static_cast<int32>(Candidates.size()) - 1;
        }
    }

    if (ChosenIndex < 0 || BestFinal < ActThreshold)
    {
        ChosenIndex = -1;
        return FName::None;
    }

    Candidates[ChosenIndex].bChosen = true;
    return Candidates[ChosenIndex].ActionId;
}
