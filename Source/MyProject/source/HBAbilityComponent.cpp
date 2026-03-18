#include "source/HBAbilityComponent.h"

#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "source/HBAttributeComponent.h"

namespace
{
bool IsValidAbilitySlotValue(const EHeroAbilitySlot AbilitySlot)
{
	switch (AbilitySlot)
	{
	case EHeroAbilitySlot::AbilityOne:
	case EHeroAbilitySlot::AbilityTwo:
	case EHeroAbilitySlot::AbilityThree:
		return true;
	default:
		return false;
	}
}
}

UHBAbilityComponent::UHBAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	AbilityOneSpec.AbilityType = EHeroAbilityType::DashBoost;
	AbilityOneSpec.CooldownSeconds = 3.0f;
	AbilityOneSpec.StaminaCost = 30.0f;
	AbilityOneSpec.DashStrength = 1650.0f;
	AbilityOneSpec.DashUpwardStrength = 100.0f;
}

bool UHBAbilityComponent::TryActivateAbility(EHeroAbilitySlot AbilitySlot)
{
	if (!GetOwner() || !IsValidAbilitySlotValue(AbilitySlot))
	{
		return false;
	}

	if (!GetOwner()->HasAuthority())
	{
		ServerTryActivateAbility(AbilitySlot);
		return true;
	}

	return CommitAbilityOnServer(AbilitySlot);
}

bool UHBAbilityComponent::IsAbilityOnCooldown(EHeroAbilitySlot AbilitySlot) const
{
	if (!GetWorld())
	{
		return true;
	}

	return GetWorld()->GetTimeSeconds() < GetCooldownEndTime(AbilitySlot);
}

float UHBAbilityComponent::GetRemainingCooldown(EHeroAbilitySlot AbilitySlot) const
{
	if (!GetWorld())
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, GetCooldownEndTime(AbilitySlot) - GetWorld()->GetTimeSeconds());
}

void UHBAbilityComponent::ServerTryActivateAbility_Implementation(EHeroAbilitySlot AbilitySlot)
{
	if (!IsValidAbilitySlotValue(AbilitySlot))
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const AController* OwnerController = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	if (!OwnerPawn || !OwnerController || !OwnerController->IsPlayerController())
	{
		return;
	}

	CommitAbilityOnServer(AbilitySlot);
}

void UHBAbilityComponent::MulticastPlayAbilityEffect_Implementation(EHeroAbilitySlot AbilitySlot, APawn* ActivatingPawn, int32 ActivationId)
{
	if (ActivationId <= LastProcessedActivationId)
	{
		return;
	}

	LastProcessedActivationId = ActivationId;
	PlayAbilityMontage(ActivatingPawn, AbilitySlot);
	OnHeroAbilityActivated.Broadcast(AbilitySlot, ActivatingPawn);
}

void UHBAbilityComponent::OnRep_ReplicatedActivation()
{
	if (ReplicatedActivation.ActivationId <= LastProcessedActivationId)
	{
		return;
	}

	LastProcessedActivationId = ReplicatedActivation.ActivationId;
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	PlayAbilityMontage(OwnerPawn, ReplicatedActivation.AbilitySlot);
	OnHeroAbilityActivated.Broadcast(ReplicatedActivation.AbilitySlot, OwnerPawn);
}

const FHeroAbilitySpec& UHBAbilityComponent::GetSpec(EHeroAbilitySlot AbilitySlot) const
{
	switch (AbilitySlot)
	{
	case EHeroAbilitySlot::AbilityOne:
		return AbilityOneSpec;
	case EHeroAbilitySlot::AbilityTwo:
		return AbilityTwoSpec;
	case EHeroAbilitySlot::AbilityThree:
	default:
		return AbilityThreeSpec;
	}
}

float UHBAbilityComponent::GetCooldownEndTime(EHeroAbilitySlot AbilitySlot) const
{
	switch (AbilitySlot)
	{
	case EHeroAbilitySlot::AbilityOne:
		return AbilityOneCooldownEndTime;
	case EHeroAbilitySlot::AbilityTwo:
		return AbilityTwoCooldownEndTime;
	case EHeroAbilitySlot::AbilityThree:
	default:
		return AbilityThreeCooldownEndTime;
	}
}

void UHBAbilityComponent::SetCooldownEndTime(EHeroAbilitySlot AbilitySlot, float NewCooldownEndTime)
{
	switch (AbilitySlot)
	{
	case EHeroAbilitySlot::AbilityOne:
		AbilityOneCooldownEndTime = NewCooldownEndTime;
		break;
	case EHeroAbilitySlot::AbilityTwo:
		AbilityTwoCooldownEndTime = NewCooldownEndTime;
		break;
	case EHeroAbilitySlot::AbilityThree:
	default:
		AbilityThreeCooldownEndTime = NewCooldownEndTime;
		break;
	}
}

bool UHBAbilityComponent::CommitAbilityOnServer(EHeroAbilitySlot AbilitySlot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !GetWorld() || !IsValidAbilitySlotValue(AbilitySlot))
	{
		return false;
	}

	if (IsAbilityOnCooldown(AbilitySlot))
	{
		return false;
	}

	const FHeroAbilitySpec& Spec = GetSpec(AbilitySlot);
	UHBAttributeComponent* AttributeComponent = GetAttributeComponent();
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!AttributeComponent || !OwnerPawn)
	{
		return false;
	}

	const float CurrentStamina = AttributeComponent->GetAttributeValue(EHBHeroAttribute::Stamina);
	if (CurrentStamina < Spec.StaminaCost)
	{
		return false;
	}

	const bool bStaminaCommitted = AttributeComponent->ModifyAttribute(EHBHeroAttribute::Stamina, -Spec.StaminaCost);
	if (!bStaminaCommitted)
	{
		return false;
	}

	if (!ExecuteAbilityOnServer(AbilitySlot, OwnerPawn))
	{
		AttributeComponent->ModifyAttribute(EHBHeroAttribute::Stamina, Spec.StaminaCost);
		return false;
	}

	const float CooldownReduction = FMath::Clamp(
		AttributeComponent->GetAttributeValue(EHBHeroAttribute::AbilityCooldownReduction),
		0.0f,
		0.95f);
	const float EffectiveCooldown = FMath::Max(0.0f, Spec.CooldownSeconds * (1.0f - CooldownReduction));
	SetCooldownEndTime(AbilitySlot, GetWorld()->GetTimeSeconds() + EffectiveCooldown);

	ReplicatedActivation.AbilitySlot = AbilitySlot;
	++ReplicatedActivation.ActivationId;

	MulticastPlayAbilityEffect(AbilitySlot, OwnerPawn, ReplicatedActivation.ActivationId);
	return true;
}

bool UHBAbilityComponent::ExecuteAbilityOnServer(EHeroAbilitySlot AbilitySlot, APawn* OwnerPawn)
{
	if (!OwnerPawn)
	{
		return false;
	}

	const FHeroAbilitySpec& Spec = GetSpec(AbilitySlot);
	switch (Spec.AbilityType)
	{
	case EHeroAbilityType::DashBoost:
	{
		ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerPawn);
		if (!OwnerCharacter)
		{
			return false;
		}

		const FVector DashVelocity =
			(OwnerCharacter->GetActorForwardVector() * FMath::Max(0.0f, Spec.DashStrength)) +
			(FVector::UpVector * Spec.DashUpwardStrength);
		OwnerCharacter->LaunchCharacter(DashVelocity, true, true);
		return true;
	}
	case EHeroAbilityType::None:
	default:
		return true;
	}
}

void UHBAbilityComponent::PlayAbilityMontage(APawn* ActivatingPawn, EHeroAbilitySlot AbilitySlot) const
{
	ACharacter* Character = Cast<ACharacter>(ActivatingPawn);
	if (!Character || !Character->GetMesh())
	{
		return;
	}

	const UAnimMontage* Montage = GetSpec(AbilitySlot).ActivationMontage;
	if (!Montage)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(const_cast<UAnimMontage*>(Montage));
	}
}

UHBAttributeComponent* UHBAbilityComponent::GetAttributeComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UHBAttributeComponent>() : nullptr;
}

void UHBAbilityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UHBAbilityComponent, ReplicatedActivation, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UHBAbilityComponent, AbilityOneCooldownEndTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UHBAbilityComponent, AbilityTwoCooldownEndTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UHBAbilityComponent, AbilityThreeCooldownEndTime, COND_OwnerOnly);
}
