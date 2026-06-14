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
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Kismet/KismetMathLibrary.h"

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
		// Hitting yourself counts as an ally (self-heal)
		return true;
	}

	if (ULyraTeamSubsystem* TeamSubsystem = UWorld::GetSubsystem<ULyraTeamSubsystem>(GetWorld()))
	{
		const ELyraTeamComparison Comparison = TeamSubsystem->CompareTeams(AvatarActor, Target);
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
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return;
	}

	TSubclassOf<UGameplayEffect> EffectClass = bTargetIsAlly ? HealEffectClass : DamageEffectClass;
	const FGameplayTag& SetByCallerTag = bTargetIsAlly ? HealSetByCallerTag : DamageSetByCallerTag;

	if (!EffectClass)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, GetAbilityLevel(), EffectContext);
	if (SpecHandle.IsValid())
	{
		if (SetByCallerTag.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerTag, Magnitude);
		}

		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
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
	FCollisionShape Sphere = FCollisionShape::MakeSphere(ChainRadius);

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

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* SecondaryTarget = Overlap.GetActor();
		if (!SecondaryTarget)
		{
			continue;
		}

		const bool bSecondaryIsAlly = IsTargetAlly(SecondaryTarget, AvatarActor);

		// Chain damage only hits secondary enemies if primary was an enemy,
		// chain heal only hits secondary allies if primary was an ally.
		if (bSecondaryIsAlly != bPrimaryTargetIsAlly)
		{
			continue;
		}

		ApplyEffectToTarget(SecondaryTarget, bSecondaryIsAlly, ChainMagnitude);

		if (bShowDebugChainLines)
		{
			const FVector SecondaryLocation = SecondaryTarget->GetActorLocation();
			const FColor LineColor = bSecondaryIsAlly ? FColor::Green : FColor::Red;

			DrawDebugLine(
				World,
				ImpactPoint,
				SecondaryLocation,
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

	const bool bTargetIsAlly = IsTargetAlly(HitActor, AvatarActor);
	const float Magnitude = bTargetIsAlly
		? RollRandomInRange(MinAllyHeal, MaxAllyHeal)
		: RollRandomInRange(MinEnemyDamage, MaxEnemyDamage);

	ApplyEffectToTarget(HitActor, bTargetIsAlly, Magnitude);
	ProcessChainTargets(HitActor, AvatarActor, Hit.ImpactPoint, bTargetIsAlly, Magnitude);
}