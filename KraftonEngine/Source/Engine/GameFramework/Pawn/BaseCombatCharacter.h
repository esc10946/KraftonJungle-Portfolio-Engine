#pragma once

#include "GameFramework/Pawn/Character.h"
#include "Object/Ptr/WeakObjectPtr.h"

class UActionComponent;
class UAnimMontage;
class UAnimInstance;
class UCombatStateComponent;
class UHealthComponent;

#include "Source/Engine/GameFramework/Pawn/BaseCombatCharacter.generated.h"

UCLASS()
class ABaseCombatCharacter : public ACharacter
{
public:
	GENERATED_BODY()
	ABaseCombatCharacter() = default;
	~ABaseCombatCharacter() override = default;

	void InitDefaultComponents(const FString& SkeletalMeshFileName) override;
	void PostDuplicate() override;

	UFUNCTION(Pure, Category="Combat|Components")
	UHealthComponent* GetHealthComponent() const { return HealthComponent; }
	UFUNCTION(Pure, Category="Combat|Components")
	UCombatStateComponent* GetCombatStateComponent() const { return CombatStateComponent; }
	UFUNCTION(Pure, Category="Combat|Components")
	UActionComponent* GetActionComponent() const { return ActionComponent; }

	UFUNCTION(Callable, Category="Combat|Damage")
	void ApplyCombatDamage(float Damage, AActor* DamageCauser = nullptr, AActor* InstigatorActor = nullptr);
	UFUNCTION(Pure, Category="Combat|Damage")
	bool IsDead() const;
	UFUNCTION(Pure, Category="Combat|Damage")
	bool IsAlive() const;

	UFUNCTION(Callable, Category="Combat|Animation")
	bool PlayCombatMontage(UAnimMontage* Montage);
	UFUNCTION(Callable, Category="Combat|Animation")	
	void SetAnimGraphBool(const FName& Name, bool Value);
	UFUNCTION(Callable, Category="Combat|Animation")
	void SetAnimGraphFloat(const FName& Name, float Value);
	UFUNCTION(Callable, Category="Combat|Animation")
	void SetAnimGraphInt(const FName& Name, int32 Value);

protected:
	virtual void HandleDamaged(UHealthComponent* Component, float Damage, float NewHealth, AActor* DamageCauser, AActor* InstigatorActor);
	virtual void HandleDeath(UHealthComponent* Component, AActor* DamageCauser, AActor* InstigatorActor);
	UAnimInstance* GetAnimInstance() const;

	TWeakObjectPtr<UHealthComponent> HealthComponent = nullptr;
	TWeakObjectPtr<UCombatStateComponent> CombatStateComponent = nullptr;
	TWeakObjectPtr<UActionComponent> ActionComponent = nullptr;
};
