#pragma once

#include "Component/ActorComponent.h"
#include "Core/Delegate.h"
#include "Object/Ptr/WeakObjectPtr.h"

class AActor;
class UAnimMontage;
class UHealthComponent;
class UEnemyAttackComponent;

#include "Source/Engine/Component/AI/EnemyHitComponent.generated.h"

UENUM()
enum class EEnemyHitDirection : uint8
{
	Front = 0,
	Back = 1,
	Left = 2,
	Right = 3,
};

USTRUCT()
struct FEnemyDirectionalMontages
{
	GENERATED_BODY()

	UPROPERTY(Edit, Save, Category="EnemyHit", DisplayName="Front", Type=ObjectRef, AllowedClass=UAnimMontage)
	UAnimMontage* Front = nullptr;

	UPROPERTY(Edit, Save, Category="EnemyHit", DisplayName="Back", Type=ObjectRef, AllowedClass=UAnimMontage)
	UAnimMontage* Back = nullptr;

	UPROPERTY(Edit, Save, Category="EnemyHit", DisplayName="Left", Type=ObjectRef, AllowedClass=UAnimMontage)
	UAnimMontage* Left = nullptr;

	UPROPERTY(Edit, Save, Category="EnemyHit", DisplayName="Right", Type=ObjectRef, AllowedClass=UAnimMontage)
	UAnimMontage* Right = nullptr;
};

UCLASS()
class UEnemyHitComponent : public UActorComponent
{
public:
	GENERATED_BODY()
	UEnemyHitComponent() = default;
	~UEnemyHitComponent() override = default;

	void BeginPlay() override;
	void EndPlay() override;

	UFUNCTION(Callable, Category="EnemyAI|Hit")
	void BindHealthEvents();
	UFUNCTION(Callable, Category="EnemyAI|Hit")
	void UnbindHealthEvents();
	UFUNCTION(Pure, Category="EnemyAI|Hit")
	EEnemyHitDirection ResolveHitDirection(AActor* DamageCauser, AActor* InstigatorActor) const;
	UFUNCTION(Callable, Category="EnemyAI|Hit")
	bool PlayHitMontage(AActor* DamageCauser, AActor* InstigatorActor);
	UFUNCTION(Callable, Category="EnemyAI|Hit")
	bool PlayDeathMontage(AActor* DamageCauser, AActor* InstigatorActor);

	UPROPERTY(Edit, Save, Category="EnemyAI|Hit", DisplayName="Hit Montages")
	FEnemyDirectionalMontages HitMontages;

	UPROPERTY(Edit, Save, Category="EnemyAI|Hit", DisplayName="Death Montages")
	FEnemyDirectionalMontages DeathMontages;

	UPROPERTY(Edit, Save, Category="EnemyAI|Hit", DisplayName="Stop Current Montage Before Hit")
	bool bStopCurrentMontageBeforeHit = true;

	UPROPERTY(Edit, Save, Category="EnemyAI|Hit", DisplayName="Stop Current Montage Before Death")
	bool bStopCurrentMontageBeforeDeath = true;

	UPROPERTY(Edit, Save, Category="EnemyAI|Hit", DisplayName="Play Hit When Damage Is Zero")
	bool bPlayHitWhenDamageIsZero = false;

	UPROPERTY(Edit, Save, Category="EnemyAI|Hit", DisplayName="Restart Attack Cooldowns On Hit")
	bool bClearAttackCooldownsOnHit = true;

private:
	void HandleDamaged(UHealthComponent* Component, float Damage, float NewHealth, AActor* DamageCauser, AActor* InstigatorActor);
	void HandleDeath(UHealthComponent* Component, AActor* DamageCauser, AActor* InstigatorActor);
	void RestartAttackCooldowns();
	UAnimMontage* SelectMontage(const FEnemyDirectionalMontages& Montages, EEnemyHitDirection Direction) const;
	bool PlayDirectionalMontage(const FEnemyDirectionalMontages& Montages, EEnemyHitDirection Direction, bool bStopCurrentMontage);

	TWeakObjectPtr<UHealthComponent> BoundHealthComponent = nullptr;
	FDelegateHandle DamagedHandle;
	FDelegateHandle DeathHandle;
	bool bBoundHealthEvents = false;
};
