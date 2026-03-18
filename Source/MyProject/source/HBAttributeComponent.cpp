#include "source/HBAttributeComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

UHBAttributeComponent::UHBAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHBAttributeComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyMovementSpeedToOwner();
}

float UHBAttributeComponent::GetAttributeValue(EHBHeroAttribute Attribute) const
{
	switch (Attribute)
	{
	case EHBHeroAttribute::Health:
		return Health;
	case EHBHeroAttribute::Stamina:
		return Stamina;
	case EHBHeroAttribute::MovementSpeed:
		return MovementSpeed;
	case EHBHeroAttribute::KickPower:
		return KickPower;
	case EHBHeroAttribute::PassAccuracy:
		return PassAccuracy;
	case EHBHeroAttribute::AbilityCooldownReduction:
		return AbilityCooldownReduction;
	default:
		return 0.0f;
	}
}

bool UHBAttributeComponent::ModifyAttribute(EHBHeroAttribute Attribute, float Delta)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	return SetAttributeValueInternal(Attribute, GetAttributeValue(Attribute) + Delta);
}

bool UHBAttributeComponent::SetAttributeValue(EHBHeroAttribute Attribute, float NewValue)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	return SetAttributeValueInternal(Attribute, NewValue);
}

void UHBAttributeComponent::Server_RequestModifyAttribute_Implementation(EHBHeroAttribute Attribute, float Delta)
{
	// Raw attribute mutation from clients is intentionally disabled for anti-cheat.
	// All gameplay attribute changes must be authored by trusted server gameplay code.
	UE_LOG(LogTemp, Warning, TEXT("Server_RequestModifyAttribute rejected for %s"), *GetNameSafe(GetOwner()));
}

void UHBAttributeComponent::OnRep_Health(float OldValue)
{
	BroadcastChange(EHBHeroAttribute::Health, OldValue, Health);
}

void UHBAttributeComponent::OnRep_Stamina(float OldValue)
{
	BroadcastChange(EHBHeroAttribute::Stamina, OldValue, Stamina);
}

void UHBAttributeComponent::OnRep_MovementSpeed(float OldValue)
{
	ApplyMovementSpeedToOwner();
	BroadcastChange(EHBHeroAttribute::MovementSpeed, OldValue, MovementSpeed);
}

void UHBAttributeComponent::OnRep_KickPower(float OldValue)
{
	BroadcastChange(EHBHeroAttribute::KickPower, OldValue, KickPower);
}

void UHBAttributeComponent::OnRep_PassAccuracy(float OldValue)
{
	BroadcastChange(EHBHeroAttribute::PassAccuracy, OldValue, PassAccuracy);
}

void UHBAttributeComponent::OnRep_AbilityCooldownReduction(float OldValue)
{
	BroadcastChange(EHBHeroAttribute::AbilityCooldownReduction, OldValue, AbilityCooldownReduction);
}

void UHBAttributeComponent::BroadcastChange(EHBHeroAttribute Attribute, float OldValue, float NewValue)
{
	OnHeroAttributeChanged.Broadcast(Attribute, OldValue, NewValue);
}

bool UHBAttributeComponent::SetAttributeValueInternal(EHBHeroAttribute Attribute, float NewValue)
{
	const float SanitizedValue = SanitizeAttributeValue(Attribute, NewValue);

	switch (Attribute)
	{
	case EHBHeroAttribute::Health:
		if (FMath::IsNearlyEqual(Health, SanitizedValue))
		{
			return false;
		}
		{
			const float OldValue = Health;
			Health = SanitizedValue;
			OnRep_Health(OldValue);
		}
		return true;
	case EHBHeroAttribute::Stamina:
		if (FMath::IsNearlyEqual(Stamina, SanitizedValue))
		{
			return false;
		}
		{
			const float OldValue = Stamina;
			Stamina = SanitizedValue;
			OnRep_Stamina(OldValue);
		}
		return true;
	case EHBHeroAttribute::MovementSpeed:
		if (FMath::IsNearlyEqual(MovementSpeed, SanitizedValue))
		{
			return false;
		}
		{
			const float OldValue = MovementSpeed;
			MovementSpeed = SanitizedValue;
			OnRep_MovementSpeed(OldValue);
		}
		return true;
	case EHBHeroAttribute::KickPower:
		if (FMath::IsNearlyEqual(KickPower, SanitizedValue))
		{
			return false;
		}
		{
			const float OldValue = KickPower;
			KickPower = SanitizedValue;
			OnRep_KickPower(OldValue);
		}
		return true;
	case EHBHeroAttribute::PassAccuracy:
		if (FMath::IsNearlyEqual(PassAccuracy, SanitizedValue))
		{
			return false;
		}
		{
			const float OldValue = PassAccuracy;
			PassAccuracy = SanitizedValue;
			OnRep_PassAccuracy(OldValue);
		}
		return true;
	case EHBHeroAttribute::AbilityCooldownReduction:
		if (FMath::IsNearlyEqual(AbilityCooldownReduction, SanitizedValue))
		{
			return false;
		}
		{
			const float OldValue = AbilityCooldownReduction;
			AbilityCooldownReduction = SanitizedValue;
			OnRep_AbilityCooldownReduction(OldValue);
		}
		return true;
	default:
		return false;
	}
}

float UHBAttributeComponent::SanitizeAttributeValue(EHBHeroAttribute Attribute, float InValue) const
{
	switch (Attribute)
	{
	case EHBHeroAttribute::Health:
	case EHBHeroAttribute::Stamina:
		return FMath::Max(0.0f, InValue);
	case EHBHeroAttribute::MovementSpeed:
		return FMath::Max(100.0f, InValue);
	case EHBHeroAttribute::KickPower:
	case EHBHeroAttribute::PassAccuracy:
		return FMath::Max(0.0f, InValue);
	case EHBHeroAttribute::AbilityCooldownReduction:
		return FMath::Clamp(InValue, 0.0f, 0.95f);
	default:
		return InValue;
	}
}

void UHBAttributeComponent::ApplyMovementSpeedToOwner() const
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->GetCharacterMovement())
	{
		return;
	}

	OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
}

void UHBAttributeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHBAttributeComponent, Health);
	DOREPLIFETIME(UHBAttributeComponent, Stamina);
	DOREPLIFETIME(UHBAttributeComponent, MovementSpeed);
	DOREPLIFETIME(UHBAttributeComponent, KickPower);
	DOREPLIFETIME(UHBAttributeComponent, PassAccuracy);
	DOREPLIFETIME(UHBAttributeComponent, AbilityCooldownReduction);
}
