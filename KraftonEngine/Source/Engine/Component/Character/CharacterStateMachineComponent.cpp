#include "Component/Character/CharacterStateMachineComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/Graph/AnimGraphInstance.h"
#include "Component/Combat/CombatMoveComponent.h"
#include "Component/Combat/CombatStateComponent.h"
#include "Component/Combat/HealthComponent.h"
#include "Component/Movement/CharacterMovementComponent.h"
#include "Component/Primitive/SkeletalMeshComponent.h"
#include "GameFramework/AActor.h"
#include "GameFramework/Pawn/Character.h"
#include "Math/MathUtils.h"
#include "Math/Rotator.h"
#include "Object/Object.h"

#include <algorithm>
#include <cmath>

const FCharacterStateDef& GetCharacterStateDef(ECharacterState State)
{
    // { Locomotion, AnimStateId, bAllowsPolicyExit, bFaceTarget }
    static const FCharacterStateDef Defs[] = {
        /* Idle      */ { ELocomotionMode::InputAllowed, 0, true,  false },
        /* Approach  */ { ELocomotionMode::InputAllowed, 1, true,  true  },
        /* Strafe    */ { ELocomotionMode::Strafe,       2, true,  true  },
        /* Retreat   */ { ELocomotionMode::Retreat,      3, true,  true  },
        /* Attack    */ { ELocomotionMode::Locked,       4, false, false },
        /* Defend    */ { ELocomotionMode::Locked,       5, false, true  },
        /* Hit       */ { ELocomotionMode::Locked,       6, false, false },
        /* Staggered */ { ELocomotionMode::Locked,       7, false, false },
        /* Dead      */ { ELocomotionMode::Locked,       8, false, false },
    };
    const int32 Index = static_cast<int32>(State);
    const int32 Count = static_cast<int32>(sizeof(Defs) / sizeof(Defs[0]));
    return (Index >= 0 && Index < Count) ? Defs[Index] : Defs[0];
}

namespace
{
    float NormalizeDeltaYaw(float Delta)
    {
        while (Delta > 180.0f) Delta -= 360.0f;
        while (Delta < -180.0f) Delta += 360.0f;
        return Delta;
    }

    FVector FlatDirTo(AActor* From, AActor* To)
    {
        if (!From || !To)
        {
            return FVector::ZeroVector;
        }
        FVector Delta = To->GetActorLocation() - From->GetActorLocation();
        Delta.Z = 0.0f;
        if (Delta.IsNearlyZero())
        {
            return FVector::ZeroVector;
        }
        return Delta.Normalized();
    }
}

void UCharacterStateMachineComponent::BeginPlay()
{
    UActorComponent::BeginPlay();
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    if (!bEventsBound)
    {
        if (UHealthComponent* Health = Owner->GetComponentByClass<UHealthComponent>())
        {
            Health->OnDamaged.AddUObject(this, &UCharacterStateMachineComponent::HandleDamaged);
            Health->OnDeath.AddUObject(this, &UCharacterStateMachineComponent::HandleDeath);
        }
        if (UCombatStateComponent* Combat = Owner->GetComponentByClass<UCombatStateComponent>())
        {
            Combat->OnStaggerStarted.AddUObject(this, &UCharacterStateMachineComponent::HandleStaggerStarted);
            Combat->OnStaggerEnded.AddUObject(this, &UCharacterStateMachineComponent::HandleStaggerEnded);
        }
        bEventsBound = true;
    }

    if (ACharacter* Character = Cast<ACharacter>(Owner))
    {
        if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
        {
            bCachedOrientToMovement = Movement->bOrientRotationToMovement;
            bHasCachedOrient        = true;
        }
    }
}

void UCharacterStateMachineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction)
{
    UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

    TimeInState     += DeltaTime;
    StrafeFlipTimer += DeltaTime;
    if (StrafeFlipPeriod > 0.0f && StrafeFlipTimer >= StrafeFlipPeriod)
    {
        bStrafeClockwise = !bStrafeClockwise;
        StrafeFlipTimer  = 0.0f;
    }
    if (DashRemaining > 0.0f)
    {
        DashRemaining = (std::max)(0.0f, DashRemaining - DeltaTime);
    }

    CheckAutoExit(GetCharacterStateDef(CurrentState), DeltaTime); // may ChangeState

    const FCharacterStateDef& Def = GetCharacterStateDef(CurrentState);
    ApplyMovement(Def, DeltaTime);
    ApplyFacing(Def, DeltaTime);
    ApplyAnimation(Def);
}

bool UCharacterStateMachineComponent::RequestState(ECharacterState NewState)
{
    if (CurrentState == ECharacterState::Dead)
    {
        return false;
    }
    const FCharacterStateDef& Def = GetCharacterStateDef(CurrentState);
    if (!Def.bAllowsPolicyExit && CurrentState != NewState)
    {
        return false; // committed 상태 — 정책이 떠날 수 없음
    }
    if (CurrentState == NewState)
    {
        return true;
    }
    ChangeState(NewState);
    return true;
}

void UCharacterStateMachineComponent::ForceState(ECharacterState NewState)
{
    if (CurrentState != NewState)
    {
        ChangeState(NewState);
    }
}

void UCharacterStateMachineComponent::ChangeState(ECharacterState NewState)
{
    PreviousState = CurrentState;
    CurrentState  = NewState;
    TimeInState   = 0.0f;
    OnStateChanged.Broadcast(this, PreviousState, NewState);
}

void UCharacterStateMachineComponent::BeginDash(float Seconds, float Scale)
{
    DashRemaining = (std::max)(0.0f, Seconds);
    DashScale     = Scale;
}

void UCharacterStateMachineComponent::ApplyMovement(const FCharacterStateDef& Def, float /*DeltaTime*/)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character)
    {
        return;
    }
    UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
    if (!Movement)
    {
        return;
    }
    AActor* Target = MovementTarget.Get();

    switch (Def.Locomotion)
    {
    case ELocomotionMode::Locked:
        if (DashRemaining > 0.0f && Target)
        {
            // 갭클로저 대시 — Locked 라도 짧게 타깃 방향으로 전진(root motion 없는 돌진 공격용).
            const FVector Dir = FlatDirTo(Character, Target);
            if (!Dir.IsNearlyZero())
            {
                Character->AddMovementInput(Dir, DashScale);
            }
        }
        // root motion 이 있으면 그것이 변위를 담당하므로 건드리지 않는다(montage 이동 보존).
        else if (!Movement->HasPendingRootMotion())
        {
            Movement->ClearInputVector();
            Movement->StopHorizontalMovementImmediately();
        }
        break;
    case ELocomotionMode::InputAllowed:
        break; // 외부 입력(WASD/path-follow)이 이동을 구동 — 미개입
    case ELocomotionMode::Strafe:
        if (Target)
        {
            const FVector Dir = FlatDirTo(Character, Target);
            if (!Dir.IsNearlyZero())
            {
                FVector Perp(-Dir.Y, Dir.X, 0.0f);
                if (!bStrafeClockwise)
                {
                    Perp = Perp * -1.0f;
                }
                Character->AddMovementInput(Perp, StrafeScale);
            }
        }
        break;
    case ELocomotionMode::Retreat:
        if (Target)
        {
            const FVector Dir = FlatDirTo(Character, Target);
            if (!Dir.IsNearlyZero())
            {
                Character->AddMovementInput(Dir * -1.0f, RetreatScale);
            }
        }
        break;
    }
}

void UCharacterStateMachineComponent::ApplyFacing(const FCharacterStateDef& Def, float DeltaTime)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character)
    {
        return;
    }
    UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
    AActor* Target = MovementTarget.Get();

    if (Def.bFaceTarget && Target)
    {
        // 상태머신이 타깃을 조준 — 이동방향 자동회전과 충돌하지 않게 잠시 끈다.
        if (Movement)
        {
            Movement->bOrientRotationToMovement = false;
        }
        const FVector Dir = FlatDirTo(Character, Target);
        if (!Dir.IsNearlyZero())
        {
            FRotator Rotation = Character->GetActorRotation();
            const float TargetYaw = atan2f(Dir.Y, Dir.X) * FMath::RadToDeg;
            float DeltaYaw = NormalizeDeltaYaw(TargetYaw - Rotation.Yaw);
            const float MaxStep = FaceYawRate * DeltaTime;
            DeltaYaw = FMath::Clamp(DeltaYaw, -MaxStep, MaxStep);
            Rotation.Yaw += DeltaYaw;
            Character->SetActorRotation(Rotation);
        }
    }
    else if (Movement && bHasCachedOrient)
    {
        // 조준 안 함 → 캐시된 이동방향 자동회전 정책 복원.
        Movement->bOrientRotationToMovement = bCachedOrientToMovement;
    }
}

void UCharacterStateMachineComponent::ApplyAnimation(const FCharacterStateDef& Def)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character)
    {
        return;
    }
    USkeletalMeshComponent* Mesh = Character->GetMesh();
    UAnimInstance* Anim = Mesh ? Mesh->GetAnimInstance() : nullptr;
    if (UAnimGraphInstance* Graph = Cast<UAnimGraphInstance>(Anim))
    {
        // 그래프가 이 변수들을 선언하지 않으면 no-op → 기존 그래프 비파괴.
        Graph->SetGraphVariableInt(FName("StateId"), Def.AnimStateId);
        Graph->SetGraphVariableBool(FName("IsAttacking"), CurrentState == ECharacterState::Attack);
        Graph->SetGraphVariableBool(FName("IsDefending"), CurrentState == ECharacterState::Defend);
        Graph->SetGraphVariableBool(FName("IsStaggered"), CurrentState == ECharacterState::Staggered);
    }
}

void UCharacterStateMachineComponent::CheckAutoExit(const FCharacterStateDef& /*Def*/, float /*DeltaTime*/)
{
    AActor* Owner = GetOwner();
    switch (CurrentState)
    {
    case ECharacterState::Attack:
    {
        UCombatMoveComponent* Move = Owner ? Owner->GetComponentByClass<UCombatMoveComponent>() : nullptr;
        const bool bMoveActive = Move && Move->IsMoveActive();
        bool bMontagePlaying = false;
        if (ACharacter* Character = Cast<ACharacter>(Owner))
        {
            if (USkeletalMeshComponent* Mesh = Character->GetMesh())
            {
                if (UAnimInstance* Anim = Mesh->GetAnimInstance())
                {
                    bMontagePlaying = Anim->IsMontagePlaying();
                }
            }
        }
        if ((!bMoveActive && !bMontagePlaying && TimeInState > 0.06f) || TimeInState > AttackTimeoutFallback)
        {
            ForceState(ECharacterState::Idle);
        }
        break;
    }
    case ECharacterState::Defend:
        if (TimeInState > DefendDuration)
        {
            ForceState(ECharacterState::Idle);
        }
        break;
    case ECharacterState::Hit:
        if (TimeInState > HitDuration)
        {
            ForceState(ECharacterState::Idle);
        }
        break;
    case ECharacterState::Staggered:
    {
        UCombatStateComponent* Combat = Owner ? Owner->GetComponentByClass<UCombatStateComponent>() : nullptr;
        if (Combat)
        {
            if (!Combat->IsStaggered())
            {
                ForceState(ECharacterState::Idle);
            }
        }
        else if (TimeInState > 1.0f)
        {
            ForceState(ECharacterState::Idle);
        }
        break;
    }
    default:
        break;
    }
}

void UCharacterStateMachineComponent::HandleDamaged(UHealthComponent* /*Component*/, float Damage, float /*NewHealth*/, AActor* /*DamageCauser*/, AActor* /*InstigatorActor*/)
{
    if (CurrentState == ECharacterState::Dead || CurrentState == ECharacterState::Staggered || Damage <= 0.0f)
    {
        return;
    }
    if (AActor* Owner = GetOwner())
    {
        if (UCombatStateComponent* Combat = Owner->GetComponentByClass<UCombatStateComponent>())
        {
            if (Combat->HasSuperArmor())
            {
                return; // 슈퍼아머 중에는 피격 반응 없음
            }
        }
    }
    ForceState(ECharacterState::Hit);
}

void UCharacterStateMachineComponent::HandleDeath(UHealthComponent* /*Component*/, AActor* /*DamageCauser*/, AActor* /*InstigatorActor*/)
{
    ForceState(ECharacterState::Dead);
}

void UCharacterStateMachineComponent::HandleStaggerStarted(UCombatStateComponent* /*Component*/, float /*Duration*/)
{
    if (CurrentState != ECharacterState::Dead)
    {
        ForceState(ECharacterState::Staggered);
    }
}

void UCharacterStateMachineComponent::HandleStaggerEnded(UCombatStateComponent* /*Component*/)
{
    if (CurrentState == ECharacterState::Staggered)
    {
        ForceState(ECharacterState::Idle);
    }
}
