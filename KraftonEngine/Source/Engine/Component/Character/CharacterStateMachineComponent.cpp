#include "Component/Character/CharacterStateMachineComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/Graph/AnimGraphInstance.h"
#include "Component/Movement/CharacterMovementComponent.h"
#include "Component/Primitive/SkeletalMeshComponent.h"
#include "GameFramework/AActor.h"
#include "GameFramework/Pawn/Character.h"
#include "Math/MathUtils.h"
#include "Math/Rotator.h"

#include <algorithm>
#include <cmath>

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

    ELocomotionMode SanitizeLocomotionMode(int32 Value)
    {
        switch (Value)
        {
        case static_cast<int32>(ELocomotionMode::Locked):
            return ELocomotionMode::Locked;
        case static_cast<int32>(ELocomotionMode::InputAllowed):
            return ELocomotionMode::InputAllowed;
        case static_cast<int32>(ELocomotionMode::Strafe):
            return ELocomotionMode::Strafe;
        case static_cast<int32>(ELocomotionMode::Retreat):
            return ELocomotionMode::Retreat;
        default:
            return ELocomotionMode::InputAllowed;
        }
    }
}

void UCharacterStateMachineComponent::EnsureStateDefinitions()
{
    const FName SafeInitial = InitialState.IsValid() ? InitialState : FName("Idle");
    if (!CurrentState.IsValid())
    {
        CurrentState = SafeInitial;
    }
    if (!HasStateDefinition(SafeInitial))
    {
        FCharacterStateDef Def;
        Def.StateName = SafeInitial;
        Def.Locomotion = ELocomotionMode::InputAllowed;
        Def.AnimStateId = 0;
        Def.bAllowsPolicyExit = true;
        Def.bFaceTarget = false;
        StateDefinitions.push_back(Def);
    }
}

FCharacterStateDef UCharacterStateMachineComponent::GetStateDef(const FName& StateName) const
{
    for (const FCharacterStateDef& Def : StateDefinitions)
    {
        if (Def.StateName == StateName)
        {
            return Def;
        }
    }

    FCharacterStateDef NeutralDef;
    NeutralDef.StateName = StateName.IsValid() ? StateName : InitialState;
    NeutralDef.Locomotion = ELocomotionMode::InputAllowed;
    NeutralDef.AnimStateId = 0;
    NeutralDef.bAllowsPolicyExit = true;
    NeutralDef.bFaceTarget = false;
    return NeutralDef;
}

int32 UCharacterStateMachineComponent::GetLocomotionModeInt() const
{
    return static_cast<int32>(GetStateDef(CurrentState).Locomotion);
}

bool UCharacterStateMachineComponent::IsMovementLocked() const
{
    return GetStateDef(CurrentState).Locomotion == ELocomotionMode::Locked;
}

bool UCharacterStateMachineComponent::CanPolicyExitCurrentState() const
{
    return GetStateDef(CurrentState).bAllowsPolicyExit;
}

bool UCharacterStateMachineComponent::HasStateDefinition(const FName& StateName) const
{
    for (const FCharacterStateDef& Def : StateDefinitions)
    {
        if (Def.StateName == StateName)
        {
            return true;
        }
    }
    return false;
}

void UCharacterStateMachineComponent::DefineState(const FName& StateName, int32 LocomotionMode, int32 AnimStateId, bool bAllowsPolicyExit, bool bFaceTarget)
{
    if (!StateName.IsValid())
    {
        return;
    }
    FCharacterStateDef NewDef;
    NewDef.StateName = StateName;
    NewDef.Locomotion = SanitizeLocomotionMode(LocomotionMode);
    NewDef.AnimStateId = AnimStateId;
    NewDef.bAllowsPolicyExit = bAllowsPolicyExit;
    NewDef.bFaceTarget = bFaceTarget;

    for (FCharacterStateDef& Def : StateDefinitions)
    {
        if (Def.StateName == StateName)
        {
            Def = NewDef;
            return;
        }
    }
    StateDefinitions.push_back(NewDef);
}

void UCharacterStateMachineComponent::ClearStateDefinitions()
{
    StateDefinitions.clear();
}

void UCharacterStateMachineComponent::BeginPlay()
{
    UActorComponent::BeginPlay();
    EnsureStateDefinitions();
    CurrentState = InitialState.IsValid() ? InitialState : FName("Idle");
    PreviousState = FName::None;
    TimeInState = 0.0f;

    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
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

    const FCharacterStateDef Def = GetStateDef(CurrentState);
    ApplyMovement(Def, DeltaTime);
    ApplyFacing(Def, DeltaTime);
    ApplyAnimation(Def);
}

bool UCharacterStateMachineComponent::RequestState(const FName& NewState)
{
    if (!NewState.IsValid())
    {
        return false;
    }
    const FCharacterStateDef Def = GetStateDef(CurrentState);
    if (!Def.bAllowsPolicyExit && CurrentState != NewState)
    {
        return false;
    }
    if (CurrentState == NewState)
    {
        return true;
    }
    ChangeState(NewState);
    return true;
}

void UCharacterStateMachineComponent::ForceState(const FName& NewState)
{
    if (NewState.IsValid() && CurrentState != NewState)
    {
        ChangeState(NewState);
    }
}

void UCharacterStateMachineComponent::ChangeState(const FName& NewState)
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
            const FVector Dir = FlatDirTo(Character, Target);
            if (!Dir.IsNearlyZero())
            {
                Character->AddMovementInput(Dir, DashScale);
            }
        }
        else if (!Movement->HasPendingRootMotion())
        {
            Movement->ClearInputVector();
            Movement->StopHorizontalMovementImmediately();
        }
        break;
    case ELocomotionMode::InputAllowed:
        break;
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
        Graph->SetGraphVariableInt(FName("StateId"), Def.AnimStateId);
    }
}
