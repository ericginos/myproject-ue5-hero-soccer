#include "source/GoalieHero.h"

#include "Animation/AnimInstance.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "source/GoalShieldBarrier.h"
#include "source/HBAttributeComponent.h"
#include "source/HBSoccerBall.h"

AGoalieHero::AGoalieHero()
{
	if (AttributeComponent)
	{
		AttributeComponent->Health = GoalieStartingHealth;
		AttributeComponent->MovementSpeed = GoalieBaseMovementSpeed;
	}
}

void AGoalieHero::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && AttributeComponent)
	{
		AttributeComponent->SetAttributeValue(EHBHeroAttribute::Health, GoalieStartingHealth);
		AttributeComponent->SetAttributeValue(EHBHeroAttribute::MovementSpeed, GoalieBaseMovementSpeed);
	}
}

void AGoalieHero::ActivateAbility(EHeroAbilitySlot AbilitySlot)
{
	switch (AbilitySlot)
	{
	case EHeroAbilitySlot::AbilityOne:
		UseDiveSave(FVector::ZeroVector);
		break;
	case EHeroAbilitySlot::AbilityTwo:
		UseGoalShield();
		break;
	case EHeroAbilitySlot::AbilityThree:
		UsePowerPunt();
		break;
	default:
		Super::ActivateAbility(AbilitySlot);
		break;
	}
}

void AGoalieHero::UseDiveSave(FVector DiveDirection)
{
	if (!HasAuthority())
	{
		ServerUseDiveSave(DiveDirection.GetSafeNormal());
		return;
	}

	if (!IsCooldownReady(NextDiveSaveAllowedTime) || !SpendStamina(DiveSaveStaminaCost))
	{
		return;
	}

	const FVector SafeDirection = ResolveDiveDirection(DiveDirection);
	const FVector LaunchVelocity =
		(SafeDirection * FMath::Max(0.0f, DiveSaveLaunchStrength)) +
		(FVector::UpVector * DiveSaveUpwardStrength);

	LaunchCharacter(LaunchVelocity, true, true);
	NextDiveSaveAllowedTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, DiveSaveCooldown);
	MulticastDiveSaveEffect(SafeDirection);
}

void AGoalieHero::UseGoalShield()
{
	if (!HasAuthority())
	{
		ServerUseGoalShield();
		return;
	}

	if (!IsCooldownReady(NextGoalShieldAllowedTime))
	{
		return;
	}

	if (!GoalShieldClass || ActiveGoalShield)
	{
		return;
	}

	if (!SpendStamina(GoalShieldStaminaCost))
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector SpawnLocation = GetActorLocation() + (GetActorForwardVector() * GoalShieldForwardOffset);
	const FRotator SpawnRotation = FRotator(0.0f, GetActorRotation().Yaw, 0.0f);

	AGoalShieldBarrier* SpawnedShield = GetWorld()->SpawnActor<AGoalShieldBarrier>(
		GoalShieldClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams);

	if (!SpawnedShield)
	{
		AttributeComponent->ModifyAttribute(EHBHeroAttribute::Stamina, GoalShieldStaminaCost);
		return;
	}

	ActiveGoalShield = SpawnedShield;
	ActiveGoalShield->OnDestroyed.AddDynamic(this, &AGoalieHero::HandleGoalShieldDestroyed);
	ActiveGoalShield->ActivateBarrier(GoalShieldDuration);

	NextGoalShieldAllowedTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, GoalShieldCooldown);
	MulticastGoalShieldEffect(true);
}

void AGoalieHero::UsePowerPunt()
{
	if (!HasAuthority())
	{
		ServerUsePowerPunt();
		return;
	}

	if (!IsCooldownReady(NextPowerPuntAllowedTime))
	{
		return;
	}

	AHBSoccerBall* TargetBall = FindNearestBallForPowerPunt();
	if (!TargetBall || !SpendStamina(PowerPuntStaminaCost))
	{
		return;
	}

	const FVector PuntDirection = (GetActorForwardVector() + (FVector::UpVector * PowerPuntUpwardBlend)).GetSafeNormal();
	const bool bKicked = TargetBall->TryKickFromCharacter(this, PuntDirection, PowerPuntImpulseStrength);
	if (!bKicked)
	{
		AttributeComponent->ModifyAttribute(EHBHeroAttribute::Stamina, PowerPuntStaminaCost);
		return;
	}

	NextPowerPuntAllowedTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, PowerPuntCooldown);
	MulticastPowerPuntEffect(TargetBall);
}

void AGoalieHero::ServerUseDiveSave_Implementation(FVector_NetQuantizeNormal DiveDirection)
{
	UseDiveSave(FVector(DiveDirection));
}

void AGoalieHero::ServerUseGoalShield_Implementation()
{
	UseGoalShield();
}

void AGoalieHero::ServerUsePowerPunt_Implementation()
{
	UsePowerPunt();
}

void AGoalieHero::MulticastDiveSaveEffect_Implementation(FVector_NetQuantizeNormal DiveDirection)
{
	PlayAbilityMontage(DiveSaveMontage);
}

void AGoalieHero::MulticastGoalShieldEffect_Implementation(bool bShieldActivated)
{
	if (bShieldActivated)
	{
		PlayAbilityMontage(GoalShieldMontage);
	}
}

void AGoalieHero::MulticastPowerPuntEffect_Implementation(AHBSoccerBall* PuntedBall)
{
	PlayAbilityMontage(PowerPuntMontage);
}

void AGoalieHero::OnRep_ActiveGoalShield()
{
	// Hook for UI/cosmetic shield state changes if needed.
}

void AGoalieHero::HandleGoalShieldDestroyed(AActor* DestroyedActor)
{
	if (!HasAuthority())
	{
		return;
	}

	if (DestroyedActor == ActiveGoalShield)
	{
		ActiveGoalShield = nullptr;
		MulticastGoalShieldEffect(false);
	}
}

bool AGoalieHero::IsCooldownReady(float NextAllowedServerTime) const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() >= NextAllowedServerTime;
}

bool AGoalieHero::SpendStamina(float StaminaCost)
{
	if (!AttributeComponent)
	{
		return false;
	}

	const float CurrentStamina = AttributeComponent->GetAttributeValue(EHBHeroAttribute::Stamina);
	if (CurrentStamina < StaminaCost)
	{
		return false;
	}

	return AttributeComponent->ModifyAttribute(EHBHeroAttribute::Stamina, -StaminaCost);
}

FVector AGoalieHero::ResolveDiveDirection(const FVector& RequestedDirection) const
{
	if (!RequestedDirection.IsNearlyZero())
	{
		return RequestedDirection.GetSafeNormal();
	}

	const FVector LastInput = GetLastMovementInputVector();
	return LastInput.IsNearlyZero() ? GetActorForwardVector() : LastInput.GetSafeNormal();
}

AHBSoccerBall* AGoalieHero::FindNearestBallForPowerPunt() const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	AHBSoccerBall* BestBall = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();
	const float MaxDistanceSq = FMath::Square(PowerPuntBallSearchRadius);

	for (TActorIterator<AHBSoccerBall> It(GetWorld()); It; ++It)
	{
		AHBSoccerBall* Ball = *It;
		if (!Ball)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(GetActorLocation(), Ball->GetActorLocation());
		if (DistSq > MaxDistanceSq || DistSq >= BestDistanceSq)
		{
			continue;
		}

		BestDistanceSq = DistSq;
		BestBall = Ball;
	}

	return BestBall;
}

void AGoalieHero::PlayAbilityMontage(UAnimMontage* MontageToPlay)
{
	if (!MontageToPlay || !GetMesh())
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(MontageToPlay);
	}
}

void AGoalieHero::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGoalieHero, ActiveGoalShield);
	DOREPLIFETIME_CONDITION(AGoalieHero, NextDiveSaveAllowedTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AGoalieHero, NextGoalShieldAllowedTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AGoalieHero, NextPowerPuntAllowedTime, COND_OwnerOnly);
}
