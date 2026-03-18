#include "source/ForwardHero.h"

#include "Animation/AnimInstance.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "source/HBAttributeComponent.h"
#include "source/HBSoccerBall.h"

AForwardHero::AForwardHero()
{
	if (AttributeComponent)
	{
		AttributeComponent->Health = ForwardStartingHealth;
		AttributeComponent->MovementSpeed = ForwardBaseMovementSpeed;
		AttributeComponent->KickPower = ForwardKickPower;
	}
}

void AForwardHero::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && AttributeComponent)
	{
		AttributeComponent->SetAttributeValue(EHBHeroAttribute::Health, ForwardStartingHealth);
		AttributeComponent->SetAttributeValue(EHBHeroAttribute::MovementSpeed, ForwardBaseMovementSpeed);
		AttributeComponent->SetAttributeValue(EHBHeroAttribute::KickPower, ForwardKickPower);
	}
}

void AForwardHero::ActivateAbility(EHeroAbilitySlot AbilitySlot)
{
	switch (AbilitySlot)
	{
	case EHeroAbilitySlot::AbilityOne:
		UsePowerShot();
		break;
	case EHeroAbilitySlot::AbilityTwo:
		UseCurveShot();
		break;
	case EHeroAbilitySlot::AbilityThree:
		UseBreakawayDash(FVector::ZeroVector);
		break;
	default:
		Super::ActivateAbility(AbilitySlot);
		break;
	}
}

void AForwardHero::UsePowerShot()
{
	if (!HasAuthority())
	{
		ServerUsePowerShot();
		return;
	}

	if (!IsCooldownReady(NextPowerShotAllowedTime))
	{
		return;
	}

	AHBSoccerBall* Ball = FindNearestBall(PowerShotBallSearchRadius);
	if (!Ball || !SpendStamina(PowerShotStaminaCost))
	{
		return;
	}

	const FVector ShotDirection = BuildShotDirection(0.0f, PowerShotUpwardBlend);
	const float ShotStrength = PowerShotImpulseStrength * GetKickPowerMultiplier();
	const bool bShotApplied = Ball->TryKickFromCharacter(this, ShotDirection, ShotStrength);
	if (!bShotApplied)
	{
		AttributeComponent->ModifyAttribute(EHBHeroAttribute::Stamina, PowerShotStaminaCost);
		return;
	}

	NextPowerShotAllowedTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, PowerShotCooldown);
	MulticastPowerShotEffect(Ball);
}

void AForwardHero::UseCurveShot()
{
	if (!HasAuthority())
	{
		ServerUseCurveShot();
		return;
	}

	if (!IsCooldownReady(NextCurveShotAllowedTime))
	{
		return;
	}

	AHBSoccerBall* Ball = FindNearestBall(CurveShotBallSearchRadius);
	if (!Ball || !SpendStamina(CurveShotStaminaCost))
	{
		return;
	}

	const FVector ShotDirection = BuildShotDirection(CurveShotSideBlend, CurveShotUpwardBlend);
	const float ShotStrength = CurveShotImpulseStrength * GetKickPowerMultiplier();
	const bool bShotApplied = Ball->TryKickFromCharacter(this, ShotDirection, ShotStrength);
	if (!bShotApplied)
	{
		AttributeComponent->ModifyAttribute(EHBHeroAttribute::Stamina, CurveShotStaminaCost);
		return;
	}

	NextCurveShotAllowedTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, CurveShotCooldown);
	MulticastCurveShotEffect(Ball);
}

void AForwardHero::UseBreakawayDash(FVector DashDirection)
{
	if (!HasAuthority())
	{
		ServerUseBreakawayDash(DashDirection.GetSafeNormal());
		return;
	}

	if (!IsCooldownReady(NextBreakawayDashAllowedTime) || !SpendStamina(BreakawayDashStaminaCost))
	{
		return;
	}

	const FVector SafeDirection = ResolveDashDirection(DashDirection);
	const FVector LaunchVelocity =
		(SafeDirection * FMath::Max(0.0f, BreakawayDashStrength)) +
		(FVector::UpVector * BreakawayDashUpwardStrength);

	LaunchCharacter(LaunchVelocity, true, true);
	NextBreakawayDashAllowedTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, BreakawayDashCooldown);
	MulticastBreakawayDashEffect(SafeDirection);
}

void AForwardHero::ServerUsePowerShot_Implementation()
{
	UsePowerShot();
}

void AForwardHero::ServerUseCurveShot_Implementation()
{
	UseCurveShot();
}

void AForwardHero::ServerUseBreakawayDash_Implementation(FVector_NetQuantizeNormal DashDirection)
{
	UseBreakawayDash(FVector(DashDirection));
}

void AForwardHero::MulticastPowerShotEffect_Implementation(AHBSoccerBall* ShotBall)
{
	PlayAbilityMontage(PowerShotMontage);
}

void AForwardHero::MulticastCurveShotEffect_Implementation(AHBSoccerBall* ShotBall)
{
	PlayAbilityMontage(CurveShotMontage);
}

void AForwardHero::MulticastBreakawayDashEffect_Implementation(FVector_NetQuantizeNormal DashDirection)
{
	PlayAbilityMontage(BreakawayDashMontage);
}

bool AForwardHero::IsCooldownReady(float NextAllowedServerTime) const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() >= NextAllowedServerTime;
}

bool AForwardHero::SpendStamina(float StaminaCost)
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

float AForwardHero::GetKickPowerMultiplier() const
{
	if (!AttributeComponent)
	{
		return 1.0f;
	}

	return FMath::Max(0.1f, AttributeComponent->GetAttributeValue(EHBHeroAttribute::KickPower));
}

AHBSoccerBall* AForwardHero::FindNearestBall(float SearchRadius) const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	AHBSoccerBall* BestBall = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();
	const float MaxDistanceSq = FMath::Square(SearchRadius);

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

FVector AForwardHero::BuildShotDirection(float SideBlend, float UpwardBlend) const
{
	const FVector Forward = GetActorForwardVector().GetSafeNormal();
	const FVector Right = GetActorRightVector().GetSafeNormal();
	const FVector InputDirection = GetLastMovementInputVector().GetSafeNormal();

	const float SideSign = InputDirection.IsNearlyZero() ? 1.0f : FMath::Sign(FVector::DotProduct(InputDirection, Right));
	const FVector CurveComponent = Right * (SideBlend * SideSign);
	const FVector UpComponent = FVector::UpVector * UpwardBlend;
	return (Forward + CurveComponent + UpComponent).GetSafeNormal();
}

FVector AForwardHero::ResolveDashDirection(const FVector& RequestedDirection) const
{
	if (!RequestedDirection.IsNearlyZero())
	{
		return RequestedDirection.GetSafeNormal();
	}

	const FVector LastInput = GetLastMovementInputVector();
	return LastInput.IsNearlyZero() ? GetActorForwardVector() : LastInput.GetSafeNormal();
}

void AForwardHero::PlayAbilityMontage(UAnimMontage* MontageToPlay)
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

void AForwardHero::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AForwardHero, NextPowerShotAllowedTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AForwardHero, NextCurveShotAllowedTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AForwardHero, NextBreakawayDashAllowedTime, COND_OwnerOnly);
}
