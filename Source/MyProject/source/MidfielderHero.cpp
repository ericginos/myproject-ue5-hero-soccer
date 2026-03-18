#include "source/MidfielderHero.h"

#include "Animation/AnimInstance.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "source/HBAttributeComponent.h"
#include "source/HBSoccerBall.h"
#include "source/HBSoccerPlayerState.h"

AMidfielderHero::AMidfielderHero()
{
	if (AttributeComponent)
	{
		AttributeComponent->Health = MidfielderStartingHealth;
		AttributeComponent->MovementSpeed = MidfielderBaseMovementSpeed;
	}
}

void AMidfielderHero::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && AttributeComponent)
	{
		AttributeComponent->SetAttributeValue(EHBHeroAttribute::Health, MidfielderStartingHealth);
		AttributeComponent->SetAttributeValue(EHBHeroAttribute::MovementSpeed, MidfielderBaseMovementSpeed);
	}
}

void AMidfielderHero::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
	{
		return;
	}

	if (bSpeedBoostAuraActive)
	{
		if (GetWorld()->GetTimeSeconds() >= SpeedBoostAuraEndServerTime)
		{
			EndSpeedBoostAura();
		}
		else
		{
			UpdateSpeedAuraRecipients();
		}
	}

	if (bStaminaRegenFieldActive)
	{
		if (GetWorld()->GetTimeSeconds() >= StaminaRegenFieldEndServerTime)
		{
			EndStaminaRegenField();
		}
		else
		{
			ProcessStaminaRegenField(DeltaSeconds);
		}
	}
}

void AMidfielderHero::ActivateAbility(EHeroAbilitySlot AbilitySlot)
{
	switch (AbilitySlot)
	{
	case EHeroAbilitySlot::AbilityOne:
		UseSpeedBoostAura();
		break;
	case EHeroAbilitySlot::AbilityTwo:
		UsePrecisionPass();
		break;
	case EHeroAbilitySlot::AbilityThree:
		UseStaminaRegenField();
		break;
	default:
		Super::ActivateAbility(AbilitySlot);
		break;
	}
}

void AMidfielderHero::UseSpeedBoostAura()
{
	if (!HasAuthority())
	{
		ServerUseSpeedBoostAura();
		return;
	}

	if (!IsCooldownReady(NextSpeedBoostAuraAllowedTime) || bSpeedBoostAuraActive)
	{
		return;
	}

	if (!SpendStamina(SpeedBoostAuraStaminaCost))
	{
		return;
	}

	bSpeedBoostAuraActive = true;
	SpeedBoostAuraEndServerTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.1f, SpeedBoostAuraDuration);
	NextSpeedBoostAuraAllowedTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, SpeedBoostAuraCooldown);

	UpdateSpeedAuraRecipients();
	MulticastSpeedBoostAuraEffect(true);
}

void AMidfielderHero::UsePrecisionPass()
{
	if (!HasAuthority())
	{
		ServerUsePrecisionPass();
		return;
	}

	if (!IsCooldownReady(NextPrecisionPassAllowedTime))
	{
		return;
	}

	AHBSoccerBall* Ball = FindNearestBallForPrecisionPass();
	ABaseHeroCharacter* TargetTeammate = GetSoftLockPassTarget();
	if (!TargetTeammate)
	{
		TargetTeammate = FindBestTeammateForPrecisionPass();
	}

	if (TargetTeammate)
	{
		const float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetTeammate->GetActorLocation());
		if (DistanceToTarget > PrecisionPassTargetRange)
		{
			TargetTeammate = nullptr;
		}
	}

	if (!Ball || !TargetTeammate)
	{
		return;
	}

	if (!SpendStamina(PrecisionPassStaminaCost))
	{
		return;
	}

	const FVector DirectTargetDirection = (TargetTeammate->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	const FVector ForwardDirection = GetActorForwardVector().GetSafeNormal();
	const FVector AssistedDirection = FMath::Lerp(ForwardDirection, DirectTargetDirection, PrecisionPassAssistBlend).GetSafeNormal();

	const bool bPassApplied = Ball->TryKickFromCharacter(this, AssistedDirection, PrecisionPassImpulseStrength);
	if (!bPassApplied)
	{
		AttributeComponent->ModifyAttribute(EHBHeroAttribute::Stamina, PrecisionPassStaminaCost);
		return;
	}

	NextPrecisionPassAllowedTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, PrecisionPassCooldown);
	MulticastPrecisionPassEffect(Ball, TargetTeammate);
}

void AMidfielderHero::UseStaminaRegenField()
{
	if (!HasAuthority())
	{
		ServerUseStaminaRegenField();
		return;
	}

	if (!IsCooldownReady(NextStaminaRegenFieldAllowedTime) || bStaminaRegenFieldActive)
	{
		return;
	}

	if (!SpendStamina(StaminaRegenFieldStaminaCost))
	{
		return;
	}

	bStaminaRegenFieldActive = true;
	StaminaRegenFieldEndServerTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.1f, StaminaRegenFieldDuration);
	NextStaminaRegenFieldAllowedTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, StaminaRegenFieldCooldown);

	MulticastStaminaRegenFieldEffect(true);
}

void AMidfielderHero::ServerUseSpeedBoostAura_Implementation()
{
	UseSpeedBoostAura();
}

void AMidfielderHero::ServerUsePrecisionPass_Implementation()
{
	UsePrecisionPass();
}

void AMidfielderHero::ServerUseStaminaRegenField_Implementation()
{
	UseStaminaRegenField();
}

void AMidfielderHero::MulticastSpeedBoostAuraEffect_Implementation(bool bAuraStarted)
{
	if (bAuraStarted)
	{
		PlayAbilityMontage(SpeedBoostAuraMontage);
	}
}

void AMidfielderHero::MulticastPrecisionPassEffect_Implementation(AHBSoccerBall* PassedBall, ABaseHeroCharacter* TargetTeammate)
{
	PlayAbilityMontage(PrecisionPassMontage);
}

void AMidfielderHero::MulticastStaminaRegenFieldEffect_Implementation(bool bFieldStarted)
{
	if (bFieldStarted)
	{
		PlayAbilityMontage(StaminaRegenFieldMontage);
	}
}

void AMidfielderHero::OnRep_SpeedBoostAuraActive()
{
	// Hook for local aura UI/VFX.
}

void AMidfielderHero::OnRep_StaminaRegenFieldActive()
{
	// Hook for local regen field UI/VFX.
}

bool AMidfielderHero::IsCooldownReady(float NextAllowedServerTime) const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() >= NextAllowedServerTime;
}

bool AMidfielderHero::SpendStamina(float StaminaCost)
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

ETeam AMidfielderHero::GetTeamForPawn(const APawn* Pawn) const
{
	if (!Pawn)
	{
		return ETeam::None;
	}

	const AHBSoccerPlayerState* SoccerState = Pawn->GetPlayerState<AHBSoccerPlayerState>();
	return SoccerState ? SoccerState->Team : ETeam::None;
}

void AMidfielderHero::UpdateSpeedAuraRecipients()
{
	if (!HasAuthority() || !bSpeedBoostAuraActive)
	{
		return;
	}

	const ETeam MyTeam = GetTeamForPawn(this);
	TSet<TWeakObjectPtr<ABaseHeroCharacter>> NewRecipients;

	for (TActorIterator<ABaseHeroCharacter> It(GetWorld()); It; ++It)
	{
		ABaseHeroCharacter* Ally = *It;
		if (!Ally)
		{
			continue;
		}

		const ETeam AllyTeam = GetTeamForPawn(Ally);
		if (MyTeam == ETeam::None || AllyTeam != MyTeam)
		{
			continue;
		}

		if (FVector::Dist(GetActorLocation(), Ally->GetActorLocation()) > SpeedBoostAuraRadius)
		{
			continue;
		}

		NewRecipients.Add(Ally);
	}

	for (const TWeakObjectPtr<ABaseHeroCharacter>& ExistingAllyPtr : SpeedAuraBuffedAllies)
	{
		ABaseHeroCharacter* ExistingAlly = ExistingAllyPtr.Get();
		if (!ExistingAlly || NewRecipients.Contains(ExistingAllyPtr))
		{
			continue;
		}

		ExistingAlly->SetExternalSpeedMultiplier(1.0f);
	}

	for (const TWeakObjectPtr<ABaseHeroCharacter>& NewAllyPtr : NewRecipients)
	{
		ABaseHeroCharacter* NewAlly = NewAllyPtr.Get();
		if (!NewAlly)
		{
			continue;
		}

		NewAlly->SetExternalSpeedMultiplier(SpeedBoostAuraMultiplier);
	}

	SpeedAuraBuffedAllies = MoveTemp(NewRecipients);
}

void AMidfielderHero::ClearSpeedAuraRecipients()
{
	for (const TWeakObjectPtr<ABaseHeroCharacter>& AllyPtr : SpeedAuraBuffedAllies)
	{
		ABaseHeroCharacter* Ally = AllyPtr.Get();
		if (!Ally)
		{
			continue;
		}

		Ally->SetExternalSpeedMultiplier(1.0f);
	}

	SpeedAuraBuffedAllies.Empty();
}

void AMidfielderHero::EndSpeedBoostAura()
{
	if (!HasAuthority())
	{
		return;
	}

	bSpeedBoostAuraActive = false;
	SpeedBoostAuraEndServerTime = 0.0f;
	ClearSpeedAuraRecipients();
	MulticastSpeedBoostAuraEffect(false);
}

void AMidfielderHero::ProcessStaminaRegenField(float DeltaSeconds)
{
	if (!HasAuthority() || !bStaminaRegenFieldActive)
	{
		return;
	}

	if (!GetWorld())
	{
		return;
	}

	const ETeam MyTeam = GetTeamForPawn(this);
	const float RegenAmount = FMath::Max(0.0f, StaminaRegenPerSecond * DeltaSeconds);

	for (TActorIterator<ABaseHeroCharacter> It(GetWorld()); It; ++It)
	{
		ABaseHeroCharacter* Ally = *It;
		if (!Ally || !Ally->AttributeComponent)
		{
			continue;
		}

		const ETeam AllyTeam = GetTeamForPawn(Ally);
		if (MyTeam == ETeam::None || AllyTeam != MyTeam)
		{
			continue;
		}

		if (FVector::Dist(GetActorLocation(), Ally->GetActorLocation()) > StaminaRegenFieldRadius)
		{
			continue;
		}

		Ally->AttributeComponent->ModifyAttribute(EHBHeroAttribute::Stamina, RegenAmount);
	}
}

void AMidfielderHero::EndStaminaRegenField()
{
	if (!HasAuthority())
	{
		return;
	}

	bStaminaRegenFieldActive = false;
	StaminaRegenFieldEndServerTime = 0.0f;
	MulticastStaminaRegenFieldEffect(false);
}

AHBSoccerBall* AMidfielderHero::FindNearestBallForPrecisionPass() const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	AHBSoccerBall* BestBall = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();
	const float MaxDistanceSq = FMath::Square(PrecisionPassBallSearchRadius);

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

ABaseHeroCharacter* AMidfielderHero::FindBestTeammateForPrecisionPass() const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	const ETeam MyTeam = GetTeamForPawn(this);
	ABaseHeroCharacter* BestTeammate = nullptr;
	float BestScore = -FLT_MAX;

	for (TActorIterator<ABaseHeroCharacter> It(GetWorld()); It; ++It)
	{
		ABaseHeroCharacter* Candidate = *It;
		if (!Candidate || Candidate == this)
		{
			continue;
		}

		if (GetTeamForPawn(Candidate) != MyTeam || MyTeam == ETeam::None)
		{
			continue;
		}

		const FVector ToCandidate = Candidate->GetActorLocation() - GetActorLocation();
		const float Distance = ToCandidate.Size();
		if (Distance > PrecisionPassTargetRange || Distance <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const FVector DirToCandidate = ToCandidate / Distance;
		const float ForwardDot = FVector::DotProduct(GetActorForwardVector().GetSafeNormal(), DirToCandidate);
		const float DistanceScore = 1.0f - FMath::Clamp(Distance / PrecisionPassTargetRange, 0.0f, 1.0f);
		const float Score = (ForwardDot * 0.7f) + (DistanceScore * 0.3f);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTeammate = Candidate;
		}
	}

	return BestTeammate;
}

void AMidfielderHero::PlayAbilityMontage(UAnimMontage* MontageToPlay)
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

void AMidfielderHero::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMidfielderHero, bSpeedBoostAuraActive);
	DOREPLIFETIME(AMidfielderHero, bStaminaRegenFieldActive);
	DOREPLIFETIME_CONDITION(AMidfielderHero, NextSpeedBoostAuraAllowedTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AMidfielderHero, NextPrecisionPassAllowedTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AMidfielderHero, NextStaminaRegenFieldAllowedTime, COND_OwnerOnly);
}
