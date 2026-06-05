#pragma once

#include "Component/ActorComponent.h"
#include "Core/Delegate.h"
#include "Core/Types/CoreTypes.h"
#include "Object/FName.h"
#include "Object/Ptr/WeakObjectPtr.h"

class AActor;
class UHealthComponent;
class UCombatStateComponent;

#include "Source/Engine/Component/Character/CharacterStateMachineComponent.generated.h"

// =============================================================================
//  UCharacterStateMachineComponent — 캐릭터 단일 상태 권위 (엔진 코어)
// =============================================================================
//  이동·애니메이션·정책(Lua BP / 플레이어 입력 / 내장 AI)이 따로 노는 문제를 해결한다.
//  이 컴포넌트가 "지금 무슨 상태인가"의 단일 진실 공급원이고, 매 틱:
//    ① 상태별 이동 정책 적용(Locked/InputAllowed/Strafe/Retreat)
//    ② 상태 id 를 AnimGraph 변수로 push(애니 FSM 이 같은 상태를 미러)
//    ③ one-shot 상태(Attack/Defend/Hit)는 montage/타임라인 종료 시 자가 복귀
//  정책은 RequestState 로 "무엇을 할지"만 전달 → 이동/애니가 그 상태에서 결정적으로 파생.
//
//  모든 ACharacter 에 부착되며, 전투 컴포넌트(Health/CombatState/CombatMove)가 있으면
//  그 이벤트(피격/경직/사망)에 구동되어 Hit/Staggered/Dead 로 자동 전이한다.
// =============================================================================

UENUM()
enum class ECharacterState : uint8
{
    Idle      = 0,  // 정지/일반 locomotion (Speed 가 Idle/Walk/Run 서브블렌드 구동)
    Approach  = 1,  // 타깃 접근 (path-follow / 전진 이동)
    Strafe    = 2,  // 타깃 주위 게걸음
    Retreat   = 3,  // 후퇴
    Attack    = 4,  // 공격 커밋 (montage + 프레임 데이터)
    Defend    = 5,  // 탄기/회피/가드
    Hit       = 6,  // 피격 반응
    Staggered = 7,  // 체간 붕괴 (인살 창)
    Dead      = 8,
};

inline const char* GCharacterStateNames[] = {
    "Idle", "Approach", "Strafe", "Retreat", "Attack", "Defend", "Hit", "Staggered", "Dead",
};
inline constexpr uint32 GCharacterStateCount = sizeof(GCharacterStateNames) / sizeof(GCharacterStateNames[0]);

UENUM()
enum class ELocomotionMode : uint8
{
    Locked       = 0,  // 수평 이동 정지(단, root motion 이 있으면 그것이 변위 담당)
    InputAllowed = 1,  // 외부 입력(WASD/path-follow)이 이동을 구동 — 미개입
    Strafe       = 2,  // 상태머신이 게걸음 입력 생성
    Retreat      = 3,  // 상태머신이 후퇴 입력 생성
};

// 상태별 정의(정적 테이블). 후속 단계에서 데이터 자산화 여지.
struct FCharacterStateDef
{
    ELocomotionMode Locomotion      = ELocomotionMode::InputAllowed;
    int32           AnimStateId     = 0;     // AnimGraph "StateId" 로 push
    bool            bAllowsPolicyExit = true; // false 면 정책이 떠날 수 없음(자가/이벤트만)
    bool            bFaceTarget     = false;  // 활성 중 타깃 조준(AI)
};

const FCharacterStateDef& GetCharacterStateDef(ECharacterState State);

DECLARE_MULTICAST_DELEGATE_ThreeParams(FCharacterStateChangedSignature, class UCharacterStateMachineComponent*, ECharacterState, ECharacterState);

UCLASS()
class UCharacterStateMachineComponent : public UActorComponent
{
public:
    GENERATED_BODY()
    UCharacterStateMachineComponent()           = default;
    ~UCharacterStateMachineComponent() override = default;

    void BeginPlay() override;
    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction) override;

    // 정책이 상태를 요청. 현재 상태가 bAllowsPolicyExit=false 면 거부(committed). 성공 시 true.
    UFUNCTION(Callable, Category="Character|State")
    bool RequestState(ECharacterState NewState);
    // 이벤트/시스템용 강제 전이(가드 무시).
    UFUNCTION(Callable, Category="Character|State")
    void ForceState(ECharacterState NewState);

    // 갭클로저 공격용 전진 대시 — Attack(Locked) 상태라도 짧은 시간 타깃 방향 이동 입력을 준다.
    // root motion 없는 돌진 공격이 실제로 거리를 좁히게 한다.
    UFUNCTION(Callable, Category="Character|State")
    void BeginDash(float Seconds, float Scale);

    UFUNCTION(Pure, Category="Character|State")
    ECharacterState GetState() const { return CurrentState; }
    UFUNCTION(Pure, Category="Character|State")
    int32 GetStateInt() const { return static_cast<int32>(CurrentState); }
    UFUNCTION(Pure, Category="Character|State")
    float GetTimeInState() const { return TimeInState; }
    UFUNCTION(Pure, Category="Character|State")
    int32 GetLocomotionModeInt() const { return static_cast<int32>(GetCharacterStateDef(CurrentState).Locomotion); }

    // Strafe/Retreat/조준이 사용할 이동 타깃(AI 가 설정; 일반 캐릭터는 null). 브레인 비의존.
    UFUNCTION(Callable, Category="Character|State")
    void SetMovementTarget(AActor* InTarget) { MovementTarget = InTarget; }
    UFUNCTION(Pure, Category="Character|State")
    AActor* GetMovementTarget() const { return MovementTarget.Get(); }

    FCharacterStateChangedSignature OnStateChanged;

    UPROPERTY(Edit, Save, Category="Character|State", DisplayName="Strafe Scale", Min=0.0f, Max=2.0f, Speed=0.05f)
    float StrafeScale = 0.7f;
    UPROPERTY(Edit, Save, Category="Character|State", DisplayName="Retreat Scale", Min=0.0f, Max=2.0f, Speed=0.05f)
    float RetreatScale = 0.6f;
    UPROPERTY(Edit, Save, Category="Character|State", DisplayName="Strafe Flip Period", Min=0.0f, Max=10.0f, Speed=0.05f)
    float StrafeFlipPeriod = 1.6f;
    UPROPERTY(Edit, Save, Category="Character|State", DisplayName="Face Yaw Rate", Min=0.0f, Max=3600.0f, Speed=5.0f)
    float FaceYawRate = 540.0f;
    UPROPERTY(Edit, Save, Category="Character|State", DisplayName="Hit Duration", Min=0.0f, Max=5.0f, Speed=0.02f)
    float HitDuration = 0.4f;
    UPROPERTY(Edit, Save, Category="Character|State", DisplayName="Defend Duration", Min=0.0f, Max=5.0f, Speed=0.02f)
    float DefendDuration = 0.35f;
    UPROPERTY(Edit, Save, Category="Character|State", DisplayName="Attack Timeout Fallback", Min=0.0f, Max=10.0f, Speed=0.05f)
    float AttackTimeoutFallback = 3.0f;

private:
    void ChangeState(ECharacterState NewState);
    void ApplyMovement(const FCharacterStateDef& Def, float DeltaTime);
    void ApplyAnimation(const FCharacterStateDef& Def);
    void ApplyFacing(const FCharacterStateDef& Def, float DeltaTime);
    void CheckAutoExit(const FCharacterStateDef& Def, float DeltaTime);

    void HandleDamaged(UHealthComponent* Component, float Damage, float NewHealth, AActor* DamageCauser, AActor* InstigatorActor);
    void HandleDeath(UHealthComponent* Component, AActor* DamageCauser, AActor* InstigatorActor);
    void HandleStaggerStarted(UCombatStateComponent* Component, float Duration);
    void HandleStaggerEnded(UCombatStateComponent* Component);

    ECharacterState CurrentState  = ECharacterState::Idle;
    ECharacterState PreviousState = ECharacterState::Idle;
    float           TimeInState   = 0.0f;
    TWeakObjectPtr<AActor> MovementTarget;
    bool  bStrafeClockwise = true;
    float StrafeFlipTimer  = 0.0f;
    bool  bEventsBound     = false;
    bool  bHasCachedOrient = false;
    bool  bCachedOrientToMovement = true;
    float DashRemaining    = 0.0f;
    float DashScale        = 1.0f;
};
