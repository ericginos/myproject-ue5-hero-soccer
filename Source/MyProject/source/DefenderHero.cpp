#include "source/DefenderHero.h"

#include "Animation/AnimInstance.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "source/HBAttributeComponent.h"
#include "source/HBCrowdControlComponent.h"
#include "source/HBSoccerPlayerState.h"

ADefenderHero::ADefenderHero()
{
	if (AttributeComponent)
	{
		AttributeComponent->Health = DefenderStartingHealth;
		AttributeComponent->MovementSpeed = DefenderBaseMovementSpeed;
	}
}

void ADefenderHero::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && AttributeComponent)
	{
		AttributeComponent->SetAttributeValue(EHBHeroAttribute::Health, DefenderStartingHealth);
		AttributeComponent->SetAttributeValue(EHBHeroAttribute::MovementSpeed, DefenderBaseMovementSpeed);
	}
}

void ADefenderHero::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || !bDefensiveAuraActive)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime >= DefensiveAuraEndServerTime)
	{
		EndDefensiveAura();
		return;
	}

	UpdateDefensiveAuraRecipients();
}

void ADefenderHero::ActivateAbility(EHeroAbilitySlot AbilitySlot)
{
	switch (AbilitySlot)
	{
	case EHeroAbilitySlot::AbilityOne:
		UseBodyCheck();
		break;
	case EHeroAbilitySlot::AbilityTwo:
		UseInterceptDash(FVector::ZeroVector);
		break;
	case EHeroAbilitySlot::AbilityThree:
		UseDefensiveAura();
		break;
	default:
		Super::ActivateAbility(AbilitySlot);
		break;
	}
}

void ADefenderHero::UseBodyCheck()
{
	if (!HasAuthority())
	{
		ServerUseBodyCheck();
		return;
	}

	if (!IsCooldownReady(NextBodyCheckAllowedTime) || !SpendStamina(BodyCheckStaminaCost))
	{
		return;
	}

	const ETeam MyTeam = GetTeamForPawn(this);
	for (TActorIterator<ABaseHeroCharacter> It(GetWorld()); It; ++It)
	{
		ABaseHeroCharacter* TargetHero = *It;
		if (!TargetHero || TargetHero == this)
		{
			continue;
		}

		const ETeam TargetTeam = GetTeamForPawn(TargetHero);
		if (MyTeam != ETeam::None && TargetTeam == MyTeam)
		{
			continue;
		}

		const float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetHero->GetActorLocation());
		if (DistanceToTarget > BodyCheckRange)
		{
			continue;
		}

		FVector KnockDirection = (TargetHero->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (KnockDirection.IsNearlyZero())
		{
			KnockDirection = GetActorForwardVector().GetSafeNormal2D();
		}

		const FVector RawKnockbackVector =
			(KnockDirection * BodyCheckKnockbackStrength) +
			(FVector::UpVector * BodyCheckUpwardStrength);

		const FVector KnockbackDirection = RawKnockbackVector.GetSafeNormal();
		const float KnockbackStrength = RawKnockbackVector.Size();

		const bool bAppliedViaCC =
			TargetHero->CrowdControlComponent &&
			TargetHero->CrowdControlComponent->ApplyKnockback(KnockbackDirection, KnockbackStrength);

		if (!bAppliedViaCC)
		{
			TargetHero->LaunchCharacter(RawKnockbackVector, true, true);
		}
	}

	NextBodyCheckAllowedTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, BodyCheckCooldown);
	MulticastBodyCheckEffect();
}

void ADefenderHero::UseInterceptDash(FVector DashDirection)
{
	if (!HasAuthority())
	{
		ServerUseInterceptDash(DashDirection.GetSafeNormal());
		return;
	}

	if (!IsCooldownReady(NextInterceptDashAllowedTime) || !SpendStamina(InterceptDashStaminaCost))
	{
		return;
	}

	const FVector SafeDashDirection = ResolveDashDirection(DashDirection);
	const FVector LaunchVelocity =
		(SafeDashDirection * InterceptDashStrength) +
		(FVector::UpVector * InterceptDashUpwardStrength);

	LaunchCharacter(LaunchVelocity, true, true);
	NextInterceptDashAllowedTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, InterceptDashCooldown);
	MulticastInterceptDashEffect(SafeDashDirection);
}

void ADefenderHero::UseDefensiveAura()
{
	if (!HasAuthority())
	{
		ServerUseDefensiveAura();
		return;
	}

	if (!IsCooldownReady(NextDefensiveAuraAllowedTime) || bDefensiveAuraActive)
	{
		return;
	}

	if (!SpendStamina(DefensiveAuraStaminaCost))
	{
		return;
	}

	bDefensiveAuraActive = true;
	DefensiveAuraEndServerTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.1f, DefensiveAuraDuration);
	NextDefensiveAuraAllowedTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, DefensiveAuraCooldown);
	UpdateDefensiveAuraRecipients();
	MulticastDefensiveAuraEffect(true);
}

void ADefenderHero::ServerUseBodyCheck_Implementation()
{
	UseBodyCheck();
}

void ADefenderHero::ServerUseInterceptDash_Implementation(FVector_NetQuantizeNormal DashDirection)
{
	UseInterceptDash(FVector(DashDirection));
}

void ADefenderHero::ServerUseDefensiveAura_Implementation()
{
	UseDefensiveAura();
}

void ADefenderHero::MulticastBodyCheckEffect_Implementation()
{
	PlayAbilityMontage(BodyCheckMontage);
}

void ADefenderHero::MulticastInterceptDashEffect_Implementation(FVector_NetQuantizeNormal DashDirection)
{
	PlayAbilityMontage(InterceptDashMontage);
}

void ADefenderHero::MulticastDefensiveAuraEffect_Implementation(bool bAuraActivated)
{
	if (bAuraActivated)
	{
		PlayAbilityMontage(DefensiveAuraMontage);
	}
}

void ADefenderHero::OnRep_DefensiveAuraActive()
{
	// Hook for aura VFX or HUD state on clients.
}

bool ADefenderHero::IsCooldownReady(float NextAllowedServerTime) const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() >= NextAllowedServerTime;
}

bool ADefenderHero::SpendStamina(float StaminaCost)
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

FVector ADefenderHero::ResolveDashDirection(const FVector& RequestedDirection) const
{
	if (!RequestedDirection.IsNearlyZero())
	{
		return RequestedDirection.GetSafeNormal();
	}

	const FVector LastInput = GetLastMovementInputVector();
	return LastInput.IsNearlyZero() ? GetActorForwardVector() : LastInput.GetSafeNormal();
}

ETeam ADefenderHero::GetTeamForPawn(const APawn* Pawn) const
{
	if (!Pawn)
	{
		return ETeam::None;
	}

	const AHBSoccerPlayerState* SoccerState = Pawn->GetPlayerState<AHBSoccerPlayerState>();
	return SoccerState ? SoccerState->Team : ETeam::None;
}

void ADefenderHero::UpdateDefensiveAuraRecipients()
{
	if (!HasAuthority() || !bDefensiveAuraActive)
	{
		return;
	}

	const ETeam MyTeam = GetTeamForPawn(this);
	TSet<TWeakObjectPtr<ABaseHeroCharacter>> NewRecipients;

	for (TActorIterator<ABaseHeroCharacter> It(GetWorld()); It; ++It)
	{
		ABaseHeroCharacter* Ally = *It;
		if (!Ally || Ally == this)
		{
			continue;
		}

		const ETeam AllyTeam = GetTeamForPawn(Ally);
		if (MyTeam == ETeam::None || AllyTeam != MyTeam)
		{
			continue;
		}

		if (FVector::Dist(GetActorLocation(), Ally->GetActorLocation()) > DefensiveAuraRadius)
		{
			continue;
		}

		NewRecipients.Add(Ally);
	}

	for (const TWeakObjectPtr<ABaseHeroCharacter>& ExistingAllyPtr : AuraBuffedAllies)
	{
		ABaseHeroCharacter* ExistingAlly = ExistingAllyPtr.Get();
		if (!ExistingAlly || NewRecipients.Contains(ExistingAllyPtr))
		{
			continue;
		}

		ExistingAlly->SetIncomingDamageMultiplier(1.0f);
	}

	for (const TWeakObjectPtr<ABaseHeroCharacter>& NewAllyPtr : NewRecipients)
	{
		ABaseHeroCharacter* NewAlly = NewAllyPtr.Get();
		if (!NewAlly)
		{
			continue;
		}

		NewAlly->SetIncomingDamageMultiplier(DefensiveAuraDamageMultiplier);
	}

	AuraBuffedAllies = MoveTemp(NewRecipients);
}

void ADefenderHero::EndDefensiveAura()
{
	if (!HasAuthority())
	{
		return;
	}

	bDefensiveAuraActive = false;
	DefensiveAuraEndServerTime = 0.0f;
	ClearDefensiveAuraBuffs();
	MulticastDefensiveAuraEffect(false);
}

void ADefenderHero::ClearDefensiveAuraBuffs()
{
	for (const TWeakObjectPtr<ABaseHeroCharacter>& AllyPtr : AuraBuffedAllies)
	{
		ABaseHeroCharacter* Ally = AllyPtr.Get();
		if (!Ally)
		{
			continue;
		}

		Ally->SetIncomingDamageMultiplier(1.0f);
	}

	AuraBuffedAllies.Empty();
}

void ADefenderHero::PlayAbilityMontage(UAnimMontage* MontageToPlay)
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

void ADefenderHero::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADefenderHero, bDefensiveAuraActive);
	DOREPLIFETIME_CONDITION(ADefenderHero, NextBodyCheckAllowedTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ADefenderHero, NextInterceptDashAllowedTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ADefenderHero, NextDefensiveAuraAllowedTime, COND_OwnerOnly);
}
