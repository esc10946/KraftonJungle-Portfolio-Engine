#include "AI/BehaviorTree/Runtime/BTTasks.h"

#include "AI/BehaviorTree/Runtime/BTContext.h"
#include "Component/AI/AIBlackboardComponent.h"
#include "Core/Logging/Log.h"
#include "GameFramework/Pawn/EnemyCharacter.h"

bool EvaluateBrainCondition(const FBTContext& Context, const FName& Name)
{
    AEnemyCharacter* Enemy = Context.Enemy;
    if (!Enemy)
    {
        return false;
    }
    if (Name == FName("CanSeeTarget"))      return Enemy->Brain_CanSeeTarget();
    if (Name == FName("InAttackRange"))     return Enemy->Brain_GetDistance() <= Enemy->Brain_GetAttackRange();
    if (Name == FName("TargetThreatening")) return Enemy->Brain_TargetThreatening();
    if (Name == FName("IsAlerted"))         return Enemy->Brain_IsAlerted();
    if (Name == FName("TargetInRecovery"))  return Enemy->Brain_TargetInRecovery();
    if (Name == FName("HasLineOfSight"))    return Enemy->Brain_HasLineOfSight();
    if (Name == FName("IsInProximity"))     return Enemy->Brain_IsInProximity();
    if (Name == FName("IsUnaware"))         return Enemy->Brain_IsUnaware();
    return false;
}

EBTNodeResult FBTCondition_CanSeeTarget::Tick(const FBTContext& Context)
{
    if (!Context.Enemy)
    {
        return EBTNodeResult::Failed;
    }
    return Context.Enemy->Brain_CanSeeTarget() ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

EBTNodeResult FBTTask_Chase::Tick(const FBTContext& Context)
{
    AEnemyCharacter* Enemy = Context.Enemy;
    if (!Enemy)
    {
        return EBTNodeResult::Failed;
    }
    Enemy->Brain_Chase();
    const float Distance = Enemy->Brain_GetDistance();
    const float Range    = Enemy->Brain_GetAttackRange();
    // 사정권 안이면 추격 완료(Succeeded) → 상위 Sequence 가 Attack 단계로 진행.
    return (Distance <= Range) ? EBTNodeResult::Succeeded : EBTNodeResult::InProgress;
}

EBTNodeResult FBTTask_Attack::Tick(const FBTContext& Context)
{
    AEnemyCharacter* Enemy = Context.Enemy;
    if (!Enemy)
    {
        return EBTNodeResult::Failed;
    }
    if (Enemy->Brain_IsBusy())
    {
        // 공격 애니메이션/회복 진행 중 — 끝날 때까지 이 노드가 점유.
        return EBTNodeResult::InProgress;
    }
    Enemy->Brain_FaceTarget();
    // 선택된 공격이 없거나 재생 불가하면 Failed → 상위 Selector 가 다음 후보로 폴백.
    return Enemy->Brain_PlaySelectedAttack() ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

EBTNodeResult FBTTask_Idle::Tick(const FBTContext& Context)
{
    if (Context.Enemy)
    {
        Context.Enemy->Brain_Idle();
    }
    // 폴백 행동 — 항상 점유(다른 후보가 없을 때 계속 대기).
    return EBTNodeResult::InProgress;
}

EBTNodeResult FBTTask_Strafe::Tick(const FBTContext& Context)
{
    if (!Context.Enemy)
    {
        return EBTNodeResult::Failed;
    }
    Context.Enemy->Brain_Strafe();
    return EBTNodeResult::InProgress;
}

EBTNodeResult FBTTask_Reposition::Tick(const FBTContext& Context)
{
    if (!Context.Enemy)
    {
        return EBTNodeResult::Failed;
    }
    Context.Enemy->Brain_Reposition();
    return EBTNodeResult::InProgress;
}

EBTNodeResult FBTTask_Investigate::Tick(const FBTContext& Context)
{
    if (!Context.Enemy)
    {
        return EBTNodeResult::Failed;
    }
    Context.Enemy->Brain_Investigate();
    return EBTNodeResult::InProgress;
}

EBTNodeResult FBTCondition_InAttackRange::Tick(const FBTContext& Context)
{
    if (!Context.Enemy)
    {
        return EBTNodeResult::Failed;
    }
    return (Context.Enemy->Brain_GetDistance() <= Context.Enemy->Brain_GetAttackRange())
        ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

EBTNodeResult FBTCondition_TargetThreatening::Tick(const FBTContext& Context)
{
    if (!Context.Enemy)
    {
        return EBTNodeResult::Failed;
    }
    return Context.Enemy->Brain_TargetThreatening() ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

EBTNodeResult FBTCondition_IsAlerted::Tick(const FBTContext& Context)
{
    if (!Context.Enemy)
    {
        return EBTNodeResult::Failed;
    }
    return Context.Enemy->Brain_IsAlerted() ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

EBTNodeResult FBTTask_BrainVerb::Tick(const FBTContext& Context)
{
    AEnemyCharacter* Enemy = Context.Enemy;
    if (!Enemy)
    {
        return EBTNodeResult::Failed;
    }
    const FName& V = VerbId;
    // 지속 이동류 — 점유(InProgress).
    if (V == FName("Chase"))       { Enemy->Brain_Chase();       return EBTNodeResult::InProgress; }
    if (V == FName("Strafe"))      { Enemy->Brain_Strafe();      return EBTNodeResult::InProgress; }
    if (V == FName("Reposition"))  { Enemy->Brain_Reposition();  return EBTNodeResult::InProgress; }
    if (V == FName("Investigate")) { Enemy->Brain_Investigate(); return EBTNodeResult::InProgress; }
    if (V == FName("Idle"))        { Enemy->Brain_Idle();        return EBTNodeResult::InProgress; }
    // 단발 — 즉시 결과.
    if (V == FName("FaceTarget"))         { Enemy->Brain_FaceTarget();  return EBTNodeResult::Succeeded; }
    if (V == FName("OpenDeflect"))        { Enemy->Brain_OpenDeflect(); return EBTNodeResult::Succeeded; }
    if (V == FName("AcquireTarget"))      return Enemy->Brain_AcquireTarget()      ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
    if (V == FName("PlaySelectedAttack")) return Enemy->Brain_PlaySelectedAttack() ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
    return EBTNodeResult::Failed;
}

EBTNodeResult FBTCondition_Blackboard::Tick(const FBTContext& Context)
{
    if (!Context.Blackboard || !Key.IsValid())
    {
        return EBTNodeResult::Failed;
    }
    return (Context.Blackboard->GetBool(Key) == bExpected) ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

// ── 범용 Task ──
void FBTTask_Wait::OnEnter(const FBTContext& /*Context*/)
{
    Elapsed = 0.0f;
}

EBTNodeResult FBTTask_Wait::Tick(const FBTContext& Context)
{
    Elapsed += Context.DeltaTime;
    return (Elapsed >= Duration) ? EBTNodeResult::Succeeded : EBTNodeResult::InProgress;
}

EBTNodeResult FBTTask_SetBlackboardBool::Tick(const FBTContext& Context)
{
    if (!Context.Blackboard || !Key.IsValid())
    {
        return EBTNodeResult::Failed;
    }
    Context.Blackboard->SetBool(Key, Value);
    return EBTNodeResult::Succeeded;
}

EBTNodeResult FBTTask_SetBlackboardFloat::Tick(const FBTContext& Context)
{
    if (!Context.Blackboard || !Key.IsValid())
    {
        return EBTNodeResult::Failed;
    }
    Context.Blackboard->SetFloat(Key, Value);
    return EBTNodeResult::Succeeded;
}

EBTNodeResult FBTTask_Log::Tick(const FBTContext& /*Context*/)
{
    UE_LOG("[BT] %s", Message.c_str());
    return EBTNodeResult::Succeeded;
}
