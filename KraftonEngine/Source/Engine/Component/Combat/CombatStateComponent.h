#pragma once

#include "Component/ActorComponent.h"
#include "Component/Combat/CombatTypes.h"
#include "Core/Delegate.h"

#include "Source/Engine/Component/Combat/CombatStateComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FCombatStaggerSignature, class UCombatStateComponent*, float);
DECLARE_MULTICAST_DELEGATE_OneParam(FCombatStateSignature, class UCombatStateComponent*);

UCLASS()
class UCombatStateComponent : public UActorComponent
{
public:
	GENERATED_BODY()
	UCombatStateComponent() = default;
	~UCombatStateComponent() override = default;

	void BeginPlay() override;
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction) override;

	UFUNCTION(Pure, Category="Combat|State")
	bool CanReceiveDamage() const { return bDamageEnabled && !bInvincible; }
	UFUNCTION(Callable, Category="Combat|State")
	void SetDamageEnabled(bool bEnabled) { bDamageEnabled = bEnabled; }
	UFUNCTION(Callable, Category="Combat|State")
	void SetInvincible(bool bInInvincible) { bInvincible = bInInvincible; }
	UFUNCTION(Callable, Category="Combat|State")
	void SetSuperArmor(bool bInSuperArmor) { bSuperArmor = bInSuperArmor; }

	UFUNCTION(Pure, Category="Combat|State")
	bool IsInvincible() const { return bInvincible; }
	UFUNCTION(Pure, Category="Combat|State")
	bool HasSuperArmor() const { return bSuperArmor; }
	UFUNCTION(Pure, Category="Combat|State")
	bool IsStaggered() const { return bStaggered; }

	UFUNCTION(Callable, Category="Combat|Poise")
	bool ApplyPoiseDamage(float PoiseDamage);
	UFUNCTION(Callable, Category="Combat|Poise")
	void ResetPoise();
	UFUNCTION(Callable, Category="Combat|Poise")
	void StartStagger(float Duration);
	UFUNCTION(Callable, Category="Combat|Poise")
	void StopStagger();
	UFUNCTION(Pure, Category="Combat|Poise")
	float GetPoiseRatio() const;

	UFUNCTION(Callable, Category="Combat|Team")
	void SetTeam(ECombatTeam InTeam) { Team = InTeam; }
	UFUNCTION(Pure, Category="Combat|Team")
	ECombatTeam GetTeam() const { return Team; }
	UFUNCTION(Pure, Category="Combat|Team")
	bool IsHostileTo(const UCombatStateComponent* Other) const;

	UPROPERTY(Edit, Save, Category="Combat|Team", DisplayName="Team", Enum=ECombatTeam)
	ECombatTeam Team = ECombatTeam::Neutral;

	UPROPERTY(Edit, Save, Category="Combat|Damage", DisplayName="Damage Enabled")
	bool bDamageEnabled = true;

	UPROPERTY(Edit, Save, Category="Combat|Damage", DisplayName="Invincible")
	bool bInvincible = false;

	UPROPERTY(Edit, Save, Category="Combat|Poise", DisplayName="Super Armor")
	bool bSuperArmor = false;

	UPROPERTY(Edit, Save, Category="Combat|Poise", DisplayName="Max Poise", Min=0.0f, Max=100000.0f, Speed=1.0f)
	float MaxPoise = 100.0f;

	UPROPERTY(Edit, Save, Category="Combat|Poise", DisplayName="Current Poise", Min=0.0f, Max=100000.0f, Speed=1.0f)
	float CurrentPoise = 100.0f;

	UPROPERTY(Edit, Save, Category="Combat|Poise", DisplayName="Poise Recovery Per Second", Min=0.0f, Max=100000.0f, Speed=1.0f)
	float PoiseRecoveryPerSecond = 20.0f;

	UPROPERTY(Edit, Save, Category="Combat|Poise", DisplayName="Default Stagger Duration", Min=0.0f, Max=10.0f, Speed=0.05f)
	float DefaultStaggerDuration = 0.45f;

	FCombatStaggerSignature OnStaggerStarted;
	FCombatStateSignature OnStaggerEnded;

private:
	bool bStaggered = false;
	float StaggerRemainingTime = 0.0f;
};
