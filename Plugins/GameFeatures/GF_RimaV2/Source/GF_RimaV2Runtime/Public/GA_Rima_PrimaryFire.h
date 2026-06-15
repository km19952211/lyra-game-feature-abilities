// Copyright Rima Project

#pragma once

#include "CoreMinimal.h"
#include "Weapons/LyraGameplayAbility_RangedWeapon.h"
#include "GA_Rima_PrimaryFire.generated.h"

class UGameplayEffect;
class AActor;

UCLASS()
class GF_RIMAV2RUNTIME_API UGA_Rima_PrimaryFire : public ULyraGameplayAbility_RangedWeapon
{
	GENERATED_BODY()

public:
	UGA_Rima_PrimaryFire(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Rima")
	void ProcessHitResult(const FHitResult& Hit, AActor* AvatarActor);

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Damage")
	float MinEnemyDamage = 55.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Damage")
	float MaxEnemyDamage = 70.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Healing")
	float MinAllyHeal = 65.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Healing")
	float MaxAllyHeal = 80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Chain")
	float ChainRadius = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Chain")
	float ChainMultiplier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Effects")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Effects")
	TSubclassOf<UGameplayEffect> HealEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Effects")
	FGameplayTag DamageSetByCallerTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Effects")
	FGameplayTag HealSetByCallerTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Debug")
	bool bShowDebugChainLines = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Debug")
	float DebugLineDuration = 2.0f;

private:
	bool IsTargetAlly(AActor* Target, AActor* AvatarActor) const;

	void ApplyEffectToTarget(AActor* Target, bool bTargetIsAlly, float Magnitude);

	void ProcessChainTargets(
		AActor* PrimaryTarget,
		AActor* AvatarActor,
		const FVector& ImpactPoint,
		bool bPrimaryTargetIsAlly,
		float PrimaryMagnitude
	);

	static float RollRandomInRange(float Min, float Max);
};