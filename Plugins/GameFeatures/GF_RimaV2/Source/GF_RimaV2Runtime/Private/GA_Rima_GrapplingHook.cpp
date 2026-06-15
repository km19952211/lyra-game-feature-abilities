// Copyright Rima Project

#include "GA_Rima_GrapplingHook.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"

UGA_Rima_GrapplingHook::UGA_Rima_GrapplingHook(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_Rima_GrapplingHook::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	ACharacter* Character = Cast<ACharacter>(AvatarActor);

	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("RIMA_GRAPPLE: Avatar is not Character"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector FoundHookPoint = FVector::ZeroVector;

	if (!TryFindGrapplePoint(Character, FoundHookPoint))
	{
		UE_LOG(LogTemp, Warning, TEXT("RIMA_GRAPPLE: No valid grapple surface"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("RIMA_GRAPPLE: CommitAbility failed, probably cooldown"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartGrapple(Character, FoundHookPoint);
}

void UGA_Rima_GrapplingHook::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	CleanupGrapple();

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled
	);
}

bool UGA_Rima_GrapplingHook::TryFindGrapplePoint(ACharacter* Character, FVector& OutHookPoint) const
{
	if (!Character)
	{
		return false;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}

	AController* Controller = Character->GetController();

	if (!Controller)
	{
		return false;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;

	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + (ViewRotation.Vector() * MaxGrappleRange);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RimaGrappleTrace), true);
	QueryParams.AddIgnoredActor(Character);

	FHitResult Hit;

	const bool bHit = World->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	if (bShowDebug)
	{
		const FColor DebugColor = bHit ? FColor::Green : FColor::Red;

		DrawDebugLine(
			World,
			TraceStart,
			bHit ? Hit.ImpactPoint : TraceEnd,
			DebugColor,
			false,
			DebugDuration,
			0,
			2.0f
		);

		if (bHit)
		{
			DrawDebugSphere(
				World,
				Hit.ImpactPoint,
				16.0f,
				16,
				DebugColor,
				false,
				DebugDuration,
				0,
				2.0f
			);
		}
	}

	if (!bHit)
	{
		return false;
	}

	if (!IsValidGrappleSurface(Hit))
	{
		return false;
	}

	OutHookPoint = Hit.ImpactPoint;
	return true;
}

bool UGA_Rima_GrapplingHook::IsValidGrappleSurface(const FHitResult& Hit) const
{
	if (!Hit.bBlockingHit)
	{
		return false;
	}

	AActor* HitActor = Hit.GetActor();

	if (!HitActor)
	{
		return false;
	}

	if (HitActor->IsA<APawn>())
	{
		return false;
	}

	UPrimitiveComponent* HitComponent = Hit.GetComponent();

	if (!HitComponent)
	{
		return false;
	}

	if (!HitComponent->IsQueryCollisionEnabled())
	{
		return false;
	}

	return true;
}

void UGA_Rima_GrapplingHook::StartGrapple(ACharacter* Character, const FVector& InHookPoint)
{
	if (!Character)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();

	if (!MovementComponent)
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	GrapplingCharacter = Character;
	GrapplingMovementComponent = MovementComponent;
	HookPoint = InHookPoint;

	SavedMovementMode = MovementComponent->MovementMode;
	SavedCustomMovementMode = MovementComponent->CustomMovementMode;
	SavedGravityScale = MovementComponent->GravityScale;
	SavedBrakingFrictionFactor = MovementComponent->BrakingFrictionFactor;

	MovementComponent->StopMovementImmediately();
	MovementComponent->SetMovementMode(MOVE_Flying);
	MovementComponent->GravityScale = 0.0f;
	MovementComponent->BrakingFrictionFactor = 0.0f;

	bIsGrappling = true;
	GrappleStartTime = World->GetTimeSeconds();

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (GrapplingStateTag.IsValid())
		{
			ASC->AddLooseGameplayTag(GrapplingStateTag);
		}
	}

	World->GetTimerManager().SetTimer(
		GrappleTimerHandle,
		this,
		&UGA_Rima_GrapplingHook::GrappleTick,
		GrappleTickRate,
		true
	);

	UE_LOG(LogTemp, Warning, TEXT("RIMA_GRAPPLE: Started. HookPoint=%s Speed=%.1f"),
		*HookPoint.ToString(),
		GrappleSpeed);
}

void UGA_Rima_GrapplingHook::GrappleTick()
{
	if (!bIsGrappling)
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	ACharacter* Character = GrapplingCharacter.Get();
	UCharacterMovementComponent* MovementComponent = GrapplingMovementComponent.Get();

	if (!Character || !MovementComponent)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	const float ElapsedTime = World->GetTimeSeconds() - GrappleStartTime;

	if (ElapsedTime >= MaxGrappleDuration)
	{
		UE_LOG(LogTemp, Warning, TEXT("RIMA_GRAPPLE: Finished by timeout"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	const FVector CurrentLocation = Character->GetActorLocation();
	const FVector ToHook = HookPoint - CurrentLocation;
	const float DistanceToHook = ToHook.Size();

	if (DistanceToHook <= StopDistance)
	{
		UE_LOG(LogTemp, Warning, TEXT("RIMA_GRAPPLE: Finished by distance. Distance=%.1f"), DistanceToHook);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	const FVector PullDirection = ToHook.GetSafeNormal();

	MovementComponent->Velocity = PullDirection * GrappleSpeed;
}

void UGA_Rima_GrapplingHook::CleanupGrapple()
{
	if (!bIsGrappling)
	{
		return;
	}

	UWorld* World = GetWorld();

	if (World)
	{
		World->GetTimerManager().ClearTimer(GrappleTimerHandle);
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (GrapplingStateTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(GrapplingStateTag);
		}
	}

	if (UCharacterMovementComponent* MovementComponent = GrapplingMovementComponent.Get())
	{
		MovementComponent->Velocity = FVector::ZeroVector;
		MovementComponent->GravityScale = SavedGravityScale;
		MovementComponent->BrakingFrictionFactor = SavedBrakingFrictionFactor;
		MovementComponent->SetMovementMode(SavedMovementMode, SavedCustomMovementMode);
	}

	UE_LOG(LogTemp, Warning, TEXT("RIMA_GRAPPLE: Cleanup"));

	GrapplingCharacter = nullptr;
	GrapplingMovementComponent = nullptr;
	bIsGrappling = false;
}