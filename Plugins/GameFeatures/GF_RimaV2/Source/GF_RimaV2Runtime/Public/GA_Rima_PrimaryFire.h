// Copyright Rima Project

#pragma once

#include "CoreMinimal.h"
#include "Weapons/LyraGameplayAbility_RangedWeapon.h"
#include "GA_Rima_PrimaryFire.generated.h"

class UGameplayEffect;
class AActor;

/**
 * Rima's Primary Fire ability:
 * - Hitscan sniper rifle
 * - Damages enemies, heals allies
 * - Chain damage/heal to secondary targets within radius
 */
UCLASS()
class GF_RIMAV2RUNTIME_API UGA_Rima_PrimaryFire : public ULyraGameplayAbility_RangedWeapon
{
	GENERATED_BODY()

public:
	UGA_Rima_PrimaryFire(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Rima")
	void ProcessHitResult(const FHitResult& Hit, AActor* AvatarActor);
protected:
	//~UGameplayAbility interface
	
	//~End of UGameplayAbility interface

	// --- Tunable Design Values (exposed to BP/Designers) ---

	/** Minimum damage applied to a primary enemy target */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Damage")
	float MinEnemyDamage = 55.f;

	/** Maximum damage applied to a primary enemy target */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Damage")
	float MaxEnemyDamage = 70.f;

	/** Minimum healing applied to a primary ally target */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Healing")
	float MinAllyHeal = 65.f;

	/** Maximum healing applied to a primary ally target */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Healing")
	float MaxAllyHeal = 80.f;

	/** Radius (in cm) for secondary chain damage/heal around the primary impact point */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Chain")
	float ChainRadius = 500.f; // 5 meters

	/** Percentage of base damage/heal applied to secondary chain targets */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Chain")
	float ChainMultiplier = 0.5f;

	/** GameplayEffect applied to enemy targets (expects SetByCaller magnitude "Data.Damage") */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Effects")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** GameplayEffect applied to ally targets (expects SetByCaller magnitude "Data.Healing") */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Effects")
	TSubclassOf<UGameplayEffect> HealEffectClass;

	/** SetByCaller tag used for damage magnitude */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Effects")
	FGameplayTag DamageSetByCallerTag;

	/** SetByCaller tag used for healing magnitude */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Effects")
	FGameplayTag HealSetByCallerTag;

	/** Draw debug lines from primary impact to chain targets */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Debug")
	bool bShowDebugChainLines = true;

	/** How long debug lines stay visible (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Debug")
	float DebugLineDuration = 2.0f;

private:
	/** Returns true if Target is on the same team as the ability's avatar (i.e. an ally) */
	bool IsTargetAlly(AActor* Target, AActor* AvatarActor) const;

	/** Applies the appropriate GE (damage or heal) to Target with the given magnitude */
	void ApplyEffectToTarget(AActor* Target, bool bTargetIsAlly, float Magnitude);

	/** Finds secondary targets within ChainRadius of ImpactPoint and applies chain damage/heal */
	void ProcessChainTargets(AActor* PrimaryTarget, AActor* AvatarActor, const FVector& ImpactPoint, bool bPrimaryTargetIsAlly, float PrimaryMagnitude);

	/** Returns a random value between Min and Max */
	static float RollRandomInRange(float Min, float Max);
}; 
