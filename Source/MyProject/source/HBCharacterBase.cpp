#include "source/HBCharacterBase.h"

#include "Net/UnrealNetwork.h"
#include "source/HBAttributeComponent.h"
#include "source/HBSoccerBall.h"
#include "source/HBSoccerPlayerState.h"

namespace
{
bool HasAuthoritativePlayerControl(const AHBCharacterBase* HeroCharacter)
{
	if (!HeroCharacter || !HeroCharacter->HasAuthority())
	{
		return false;
	}

	const AController* HeroController = HeroCharacter->GetController();
	return HeroController && HeroController->IsPlayerController();
}
}

AHBCharacterBase::AHBCharacterBase()
{
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(55.0f);
	SetMinNetUpdateFrequency(18.0f);
	NetPriority = 2.0f;
	SetNetCullDistanceSquared(FMath::Square(20000.0f));
	AttributeComponent = CreateDefaultSubobject<UHBAttributeComponent>(TEXT("AttributeComponent"));
}

void AHBCharacterBase::KickSoccerBall(AHBSoccerBall* Ball, FVector KickDirection)
{
	if (!Ball)
	{
		return;
	}

	if (!HasAuthority())
	{
		ServerKickSoccerBall(Ball, KickDirection.GetSafeNormal());
		return;
	}

	if (!IsActionOffCooldown(NextKickAllowedServerTime))
	{
		return;
	}

	const FVector SafeDirection = KickDirection.IsNearlyZero() ? GetActorForwardVector() : KickDirection.GetSafeNormal();
	const float KickImpulseStrength = KickImpulseBase * GetKickPowerMultiplier();
	if (Ball->TryKickFromCharacter(this, SafeDirection, KickImpulseStrength))
	{
		NextKickAllowedServerTime = GetWorld()->GetTimeSeconds() + KickCooldown;
	}
}

void AHBCharacterBase::PassSoccerBall(AHBSoccerBall* Ball, AHBCharacterBase* TargetTeammate)
{
	if (!Ball || !TargetTeammate || TargetTeammate == this)
	{
		return;
	}

	if (!HasAuthority())
	{
		ServerPassSoccerBall(Ball, TargetTeammate);
		return;
	}

	if (!IsActionOffCooldown(NextPassAllowedServerTime))
	{
		return;
	}

	if (GetHeroTeam() == ETeam::None || GetHeroTeam() != TargetTeammate->GetHeroTeam())
	{
		return;
	}

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetTeammate->GetActorLocation());
	if (DistanceToTarget > PassMaxDistance)
	{
		return;
	}

	const FVector ToTarget = (TargetTeammate->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	const float AccuracyAlpha = GetPassAccuracyAlpha();
	const float MaxErrorDegrees = 12.0f;
	const float ErrorDegrees = MaxErrorDegrees * (1.0f - AccuracyAlpha);
	const float ErrorRadians = FMath::DegreesToRadians(ErrorDegrees);
	const FVector PassDirection = ErrorRadians > 0.0f ? FMath::VRandCone(ToTarget, ErrorRadians) : ToTarget;
	const float PassImpulseStrength = PassImpulseBase * GetKickPowerMultiplier();

	if (Ball->TryKickFromCharacter(this, PassDirection, PassImpulseStrength))
	{
		NextPassAllowedServerTime = GetWorld()->GetTimeSeconds() + PassCooldown;
	}
}

void AHBCharacterBase::StartChargedShot()
{
	if (!HasAuthority())
	{
		ServerStartChargedShot();
		return;
	}

	if (bIsChargingShot || !IsActionOffCooldown(NextChargedShotAllowedServerTime))
	{
		return;
	}

	bIsChargingShot = true;
	ChargedShotStartServerTime = GetWorld()->GetTimeSeconds();
}

void AHBCharacterBase::ReleaseChargedShot(AHBSoccerBall* Ball, FVector ShotDirection)
{
	if (!Ball)
	{
		return;
	}

	if (!HasAuthority())
	{
		ServerReleaseChargedShot(Ball, ShotDirection.GetSafeNormal());
		return;
	}

	if (!bIsChargingShot || ChargedShotStartServerTime < 0.0f)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const float RawChargeTime = CurrentTime - ChargedShotStartServerTime;
	const float ClampedChargeTime = FMath::Clamp(RawChargeTime, MinChargedShotTime, MaxChargedShotTime);
	const float ChargeWindow = FMath::Max(0.01f, MaxChargedShotTime - MinChargedShotTime);
	const float ChargeAlpha = FMath::Clamp((ClampedChargeTime - MinChargedShotTime) / ChargeWindow, 0.0f, 1.0f);
	const float ChargedMultiplier = FMath::Lerp(1.0f, ChargedShotMaxMultiplier, ChargeAlpha);

	const FVector SafeDirection = ShotDirection.IsNearlyZero() ? GetActorForwardVector() : ShotDirection.GetSafeNormal();
	const float ShotImpulseStrength = ChargedShotImpulseBase * GetKickPowerMultiplier() * ChargedMultiplier;

	if (Ball->TryKickFromCharacter(this, SafeDirection, ShotImpulseStrength))
	{
		NextChargedShotAllowedServerTime = CurrentTime + ChargedShotCooldown;
	}

	bIsChargingShot = false;
	ChargedShotStartServerTime = -1.0f;
}

void AHBCharacterBase::StealBallPossession(AHBSoccerBall* Ball)
{
	if (!Ball)
	{
		return;
	}

	if (!HasAuthority())
	{
		ServerStealBallPossession(Ball);
		return;
	}

	if (!IsActionOffCooldown(NextStealAllowedServerTime))
	{
		return;
	}

	if (Ball->TryStealPossession(this))
	{
		NextStealAllowedServerTime = GetWorld()->GetTimeSeconds() + StealCooldown;
	}
}

void AHBCharacterBase::ServerKickSoccerBall_Implementation(AHBSoccerBall* Ball, FVector_NetQuantizeNormal KickDirection)
{
	if (!HasAuthoritativePlayerControl(this) || !Ball)
	{
		return;
	}

	KickSoccerBall(Ball, FVector(KickDirection));
}

void AHBCharacterBase::ServerPassSoccerBall_Implementation(AHBSoccerBall* Ball, AHBCharacterBase* TargetTeammate)
{
	if (!HasAuthoritativePlayerControl(this) || !Ball || !TargetTeammate || TargetTeammate == this)
	{
		return;
	}

	PassSoccerBall(Ball, TargetTeammate);
}

void AHBCharacterBase::ServerStartChargedShot_Implementation()
{
	if (!HasAuthoritativePlayerControl(this))
	{
		return;
	}

	StartChargedShot();
}

void AHBCharacterBase::ServerReleaseChargedShot_Implementation(AHBSoccerBall* Ball, FVector_NetQuantizeNormal ShotDirection)
{
	if (!HasAuthoritativePlayerControl(this) || !Ball)
	{
		return;
	}

	ReleaseChargedShot(Ball, FVector(ShotDirection));
}

void AHBCharacterBase::ServerStealBallPossession_Implementation(AHBSoccerBall* Ball)
{
	if (!HasAuthoritativePlayerControl(this) || !Ball)
	{
		return;
	}

	StealBallPossession(Ball);
}

void AHBCharacterBase::SetTeam(ETeam NewTeam)
{
	if (!HasAuthority())
	{
		return;
	}

	Team = NewTeam;
	OnRep_Team();
}

void AHBCharacterBase::OnRep_Team()
{
	// Apply team visuals here.
}

bool AHBCharacterBase::IsActionOffCooldown(float NextAllowedServerTime) const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() >= NextAllowedServerTime;
}

float AHBCharacterBase::GetKickPowerMultiplier() const
{
	if (!AttributeComponent)
	{
		return 1.0f;
	}

	return FMath::Max(0.1f, AttributeComponent->GetAttributeValue(EHBHeroAttribute::KickPower));
}

float AHBCharacterBase::GetPassAccuracyAlpha() const
{
	if (!AttributeComponent)
	{
		return 1.0f;
	}

	return FMath::Clamp(AttributeComponent->GetAttributeValue(EHBHeroAttribute::PassAccuracy), 0.0f, 1.0f);
}

ETeam AHBCharacterBase::GetHeroTeam() const
{
	const AHBSoccerPlayerState* SoccerPlayerState = GetPlayerState<AHBSoccerPlayerState>();
	return SoccerPlayerState ? SoccerPlayerState->Team : ETeam::None;
}

void AHBCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHBCharacterBase, Team);
	DOREPLIFETIME_CONDITION(AHBCharacterBase, NextKickAllowedServerTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AHBCharacterBase, NextPassAllowedServerTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AHBCharacterBase, NextChargedShotAllowedServerTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AHBCharacterBase, NextStealAllowedServerTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AHBCharacterBase, bIsChargingShot, COND_OwnerOnly);
}
