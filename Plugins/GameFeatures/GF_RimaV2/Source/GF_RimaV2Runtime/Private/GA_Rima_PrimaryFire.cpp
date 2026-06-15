// Copyright Rima Project

#include "GA_Rima_PrimaryFire.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Teams/LyraTeamSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "Weapons/LyraRangedWeaponInstance.h"

UGA_Rima_PrimaryFire::UGA_Rima_PrimaryFire(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

float UGA_Rima_PrimaryFire::RollRandomInRange(float Min, float Max)
{
	return FMath::FRandRange(Min, Max);
}

bool UGA_Rima_PrimaryFire::IsTargetAlly(AActor* Target, AActor* AvatarActor) const
{
	if (!Target || !AvatarActor)
	{
		return false;
	}

	if (Target == AvatarActor)
	{
		return true;
	}

	if (ULyraTeamSubsystem* TeamSubsystem = UWorld::GetSubsystem<ULyraTeamSubsystem>(GetWorld()))
	{
		const ELyraTeamComparison Comparison = TeamSubsystem->CompareTeams(AvatarActor, Target);

		UE_LOG(LogTemp, Warning, TEXT("RIMA: CompareTeams(%s, %s) = %d"),
			*GetNameSafe(AvatarActor),
			*GetNameSafe(Target),
			(int32)Comparison);

		return Comparison == ELyraTeamComparison::OnSameTeam;
	}

	return false;
}

void UGA_Rima_PrimaryFire::ApplyEffectToTarget(AActor* Target, bool bTargetIsAlly, float Magnitude)
{
	if (!Target)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("RIMA: No ASC on target %s"), *GetNameSafe(Target));
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return;
	}

	TSubclassOf<UGameplayEffect> EffectClass = bTargetIsAlly ? HealEffectClass : DamageEffectClass;
	const FGameplayTag& SetByCallerTag = bTargetIsAlly ? HealSetByCallerTag : DamageSetByCallerTag;

	UE_LOG(LogTemp, Warning, TEXT("RIMA: Target=%s IsAlly=%d Magnitude=%.1f Effect=%s Tag=%s"),
		*GetNameSafe(Target),
		bTargetIsAlly ? 1 : 0,
		Magnitude,
		EffectClass ? *EffectClass->GetName() : TEXT("NULL"),
		*SetByCallerTag.ToString());

	if (!EffectClass || !SetByCallerTag.IsValid())
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, GetAbilityLevel(), EffectContext);

	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerTag, Magnitude);

	const FActiveGameplayEffectHandle ActiveHandle =
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	UE_LOG(LogTemp, Warning, TEXT("RIMA: Applied GE to %s Success=%d"),
		*GetNameSafe(Target),
		ActiveHandle.WasSuccessfullyApplied() ? 1 : 0);
}

void UGA_Rima_PrimaryFire::ProcessChainTargets(AActor* PrimaryTarget, AActor* AvatarActor, const FVector& ImpactPoint, bool bPrimaryTargetIsAlly, float PrimaryMagnitude)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float ChainMagnitude = PrimaryMagnitude * ChainMultiplier;

	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(ChainRadius);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RimaChainOverlap), false);

	if (AvatarActor)
	{
		QueryParams.AddIgnoredActor(AvatarActor);
	}

	if (PrimaryTarget)
	{
		QueryParams.AddIgnoredActor(PrimaryTarget);
	}

	World->OverlapMultiByObjectType(
		Overlaps,
		ImpactPoint,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		Sphere,
		QueryParams
	);

	UE_LOG(LogTemp, Warning, TEXT("RIMA: Chain overlap found %d results"), Overlaps.Num());

	TSet<AActor*> ProcessedActors;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* SecondaryTarget = Overlap.GetActor();

		if (!SecondaryTarget || ProcessedActors.Contains(SecondaryTarget))
		{
			continue;
		}

		ProcessedActors.Add(SecondaryTarget);

		UAbilitySystemComponent* SecondaryASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SecondaryTarget);

		if (!SecondaryASC)
		{
			continue;
		}

		const bool bSecondaryIsAlly = IsTargetAlly(SecondaryTarget, AvatarActor);

		if (bSecondaryIsAlly != bPrimaryTargetIsAlly)
		{
			continue;
		}

		ApplyEffectToTarget(SecondaryTarget, bSecondaryIsAlly, ChainMagnitude);

		if (bShowDebugChainLines)
		{
			const FColor LineColor = bSecondaryIsAlly ? FColor::Green : FColor::Red;

			DrawDebugLine(
				World,
				ImpactPoint,
				SecondaryTarget->GetActorLocation(),
				LineColor,
				false,
				DebugLineDuration,
				0,
				2.0f
			);
		}
	}
}

void UGA_Rima_PrimaryFire::ProcessHitResult(const FHitResult& Hit, AActor* AvatarActor)
{
	AActor* HitActor = Hit.GetActor();

	if (!HitActor || !AvatarActor)
	{
		return;
	}

	UAbilitySystemComponent* HitASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);

	if (!HitASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("RIMA: Hit actor %s has no ASC"), *GetNameSafe(HitActor));
		return;
	}

	const bool bTargetIsAlly = IsTargetAlly(HitActor, AvatarActor);

	const float Magnitude = bTargetIsAlly
		? RollRandomInRange(MinAllyHeal, MaxAllyHeal)
		: RollRandomInRange(MinEnemyDamage, MaxEnemyDamage);

	ApplyEffectToTarget(HitActor, bTargetIsAlly, Magnitude);

	ProcessChainTargets(
		HitActor,
		AvatarActor,
		Hit.ImpactPoint,
		bTargetIsAlly,
		Magnitude
	);
}

void UGA_Rima_PrimaryFire::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!K2_HasAuthority())
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	APawn* AvatarPawn = Cast<APawn>(AvatarActor);

	if (!AvatarActor || !AvatarPawn)
	{
		return;
	}

	ULyraRangedWeaponInstance* WeaponInstance = GetWeaponInstance();

	if (!WeaponInstance)
	{
		return;
	}

	const FTransform AimTransform =
		GetTargetingTransform(AvatarPawn, ELyraAbilityTargetingSource::WeaponTowardsFocus);

	const FVector StartTrace = AimTransform.GetTranslation();
	const FVector AimDir = AimTransform.GetUnitAxis(EAxis::X);
	const float Range = WeaponInstance->GetMaxDamageRange();
	const FVector EndTrace = StartTrace + AimDir * Range;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RimaPrimaryFire), true, AvatarActor);
	AddAdditionalTraceIgnoreActors(QueryParams);

	const ECollisionChannel TraceChannel = DetermineTraceChannel(QueryParams, false);

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		StartTrace,
		EndTrace,
		TraceChannel,
		QueryParams
	);

	UE_LOG(LogTemp, Warning, TEXT("RIMA: ActivateAbility bHit=%d HitActor=%s Range=%.1f"),
		bHit ? 1 : 0,
		bHit ? *GetNameSafe(Hit.GetActor()) : TEXT("NULL"),
		Range);

	if (bHit)
	{
		ProcessHitResult(Hit, AvatarActor);
	}
}