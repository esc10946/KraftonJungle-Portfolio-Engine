#pragma once

#include "Core/Types/CoreTypes.h"
#include "Object/Ptr/WeakObjectPtr.h"

class AActor;

// =============================================================================
//  FSquadCoordinator — 협동 AI 공격 토큰 브로커 (엔진 코어 메커니즘, Phase 3)
// =============================================================================
//  보고서 "Squad Coordinator / Attack Tokens". 다수의 적이 한 타깃을 동시에
//  난타하지 않도록, 타깃별로 "동시 공격 토큰" 수를 제한한다. 공격하려는 적은
//  먼저 토큰을 획득해야 하고(TryAcquireToken), 이미 상한만큼 나가 있으면 실패해
//  지원/측면 재배치로 빠진다 — 보고서 CanActivelyThreaten 의사코드 그대로.
//
//  교차 액터 공유 자원이므로 컴포넌트가 아닌 프로세스 싱글턴으로 둔다. 토큰은
//  게임시간 기준 self-expiring 이라 release 누락이 있어도 교착되지 않는다(강건).
// =============================================================================

struct FAttackToken
{
    TWeakObjectPtr<AActor> Holder;
    TWeakObjectPtr<AActor> Target;
    float                  ExpirySeconds = 0.0f;
};

class FSquadCoordinator
{
public:
    static FSquadCoordinator& Get();

    // Holder 가 Target 공격 토큰을 시도/획득. 이미 보유 중이면 갱신. 타깃에 이미
    // MaxAttackers 만큼(자신 제외) 나가 있으면 false. 성공 시 true.
    bool TryAcquireToken(AActor* Holder, AActor* Target, float Now, float Duration, int32 MaxAttackers);

    void ReleaseToken(AActor* Holder);

    int32 CountActiveAttackers(AActor* Target, float Now);
    // Target 의 현재 공격자들 중 Holder 의 0-based 슬롯 인덱스(없으면 -1). 측면 배치용.
    int32 GetSlotIndex(AActor* Holder, AActor* Target, float Now);
    bool  HoldsToken(AActor* Holder, AActor* Target, float Now) const;

    void Reset() { Tokens.clear(); }

private:
    FSquadCoordinator() = default;
    void Prune(float Now);

    TArray<FAttackToken> Tokens;
};
