// Copyright Rima Project

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "GA_Rima_GrapplingHook.generated.h"

class ACharacter;
class UCharacterMovementComponent;

UCLASS()
class GF_RIMAV2RUNTIME_API UGA_Rima_GrapplingHook : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Rima_GrapplingHook(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Grapple")
	float MaxGrappleRange = 2500.0f; // 25m

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Grapple")
	float GrappleSpeed = 3000.0f; // 30m/s

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Grapple")
	float StopDistance = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Grapple")
	float MaxGrappleDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Grapple")
	float GrappleTickRate = 0.02f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Tags")
	FGameplayTag GrapplingStateTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Debug")
	bool bShowDebug = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rima|Debug")
	float DebugDuration = 2.0f;

private:
	bool TryFindGrapplePoint(ACharacter* Character, FVector& OutHookPoint) const;

	bool IsValidGrappleSurface(const FHitResult& Hit) const;

	void StartGrapple(ACharacter* Character, const FVector& InHookPoint);

	void GrappleTick();

	void CleanupGrapple();

private:
	UPROPERTY()
	TObjectPtr<ACharacter> GrapplingCharacter;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> GrapplingMovementComponent;

	FVector HookPoint = FVector::ZeroVector;

	FTimerHandle GrappleTimerHandle;

	float GrappleStartTime = 0.0f;

	bool bIsGrappling = false;

	TEnumAsByte<EMovementMode> SavedMovementMode = MOVE_Walking;

	uint8 SavedCustomMovementMode = 0;

	float SavedGravityScale = 1.0f;

	float SavedBrakingFrictionFactor = 2.0f;
};