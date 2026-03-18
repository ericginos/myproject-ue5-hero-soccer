#include "source/HBCrowdControlComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "source/BaseHeroCharacter.h"

UHBCrowdControlComponent::UHBCrowdControlComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UHBCrowdControlComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyStunState(bIsStunned);
}

void UHBCrowdControlComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !GetWorld())
	{
		return;
	}

	EvaluateExpirations(GetWorld()->GetTimeSeconds());
}

bool UHBCrowdControlComponent::ApplyKnockback(FVector KnockbackDirection, float Strength)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !GetWorld())
	{
		return false;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < LastKnockbackApplyServerTime + FMath::Max(0.0f, KnockbackReapplyCooldown))
	{
		return false;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor);
	if (!OwnerCharacter)
	{
		return false;
	}

	FVector SafeDirection = KnockbackDirection.GetSafeNormal();
	if (SafeDirection.IsNearlyZero())
	{
		SafeDirection = OwnerCharacter->GetActorForwardVector().GetSafeNormal();
	}

	const float SafeStrength = FMath::Clamp(Strength, 0.0f, FMath::Max(0.0f, MaxKnockbackStrength));
	if (SafeStrength <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	OwnerCharacter->LaunchCharacter(SafeDirection * SafeStrength, true, true);
	LastKnockbackApplyServerTime = CurrentTime;
	return true;
}

bool UHBCrowdControlComponent::ApplySlow(float SlowPercent, float DurationSeconds)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !GetWorld())
	{
		return false;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < LastSlowApplyServerTime + FMath::Max(0.0f, SlowReapplyCooldown))
	{
		return false;
	}

	const float SafeSlowPercent = FMath::Clamp(SlowPercent, 0.0f, FMath::Clamp(MaxSlowPercent, 0.0f, 0.95f));
	const float SafeDuration = FMath::Clamp(DurationSeconds, 0.0f, FMath::Max(0.0f, MaxSlowDuration));
	if (SafeSlowPercent <= KINDA_SMALL_NUMBER || SafeDuration <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const float RequestedEndTime = CurrentTime + SafeDuration;
	bool bChanged = false;

	if (!bIsSlowed)
	{
		bIsSlowed = true;
		bChanged = true;
	}

	if (SafeSlowPercent > ActiveSlowPercent + KINDA_SMALL_NUMBER)
	{
		ActiveSlowPercent = SafeSlowPercent;
		bChanged = true;
	}

	if (RequestedEndTime > SlowEndServerTime + KINDA_SMALL_NUMBER)
	{
		SlowEndServerTime = RequestedEndTime;
		bChanged = true;
	}

	LastSlowApplyServerTime = CurrentTime;

	if (bChanged)
	{
		OnRep_IsSlowed();
		OnRep_ActiveSlowPercent();
	}

	return bChanged;
}

bool UHBCrowdControlComponent::ApplyStun(float DurationSeconds)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !GetWorld())
	{
		return false;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < LastStunApplyServerTime + FMath::Max(0.0f, StunReapplyCooldown))
	{
		return false;
	}

	const float SafeDuration = FMath::Clamp(DurationSeconds, 0.0f, FMath::Max(0.0f, MaxStunDuration));
	if (SafeDuration <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const float RequestedEndTime = CurrentTime + SafeDuration;
	bool bChanged = false;

	if (!bIsStunned)
	{
		bIsStunned = true;
		bChanged = true;
	}

	if (RequestedEndTime > StunEndServerTime + KINDA_SMALL_NUMBER)
	{
		StunEndServerTime = RequestedEndTime;
		bChanged = true;
	}

	LastStunApplyServerTime = CurrentTime;

	if (bChanged)
	{
		OnRep_IsStunned();
	}

	return bChanged;
}

void UHBCrowdControlComponent::ClearAllCrowdControl()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	bIsStunned = false;
	bIsSlowed = false;
	ActiveSlowPercent = 0.0f;
	StunEndServerTime = 0.0f;
	SlowEndServerTime = 0.0f;

	OnRep_IsStunned();
	OnRep_IsSlowed();
	OnRep_ActiveSlowPercent();
}

float UHBCrowdControlComponent::GetMovementSpeedMultiplier() const
{
	if (bIsStunned)
	{
		return 0.0f;
	}

	if (!bIsSlowed)
	{
		return 1.0f;
	}

	return FMath::Clamp(1.0f - ActiveSlowPercent, 0.05f, 1.0f);
}

void UHBCrowdControlComponent::OnRep_IsStunned()
{
	ApplyStunState(bIsStunned);
	NotifyOwnerCrowdControlChanged();
}

void UHBCrowdControlComponent::OnRep_IsSlowed()
{
	NotifyOwnerCrowdControlChanged();
}

void UHBCrowdControlComponent::OnRep_ActiveSlowPercent()
{
	NotifyOwnerCrowdControlChanged();
}

void UHBCrowdControlComponent::ApplyStunState(bool bShouldStun) const
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	UCharacterMovementComponent* CharacterMovement = OwnerCharacter->GetCharacterMovement();
	if (!CharacterMovement)
	{
		return;
	}

	if (bShouldStun)
	{
		CharacterMovement->StopMovementImmediately();
		CharacterMovement->DisableMovement();
		if (AController* OwnerController = OwnerCharacter->GetController())
		{
			OwnerController->SetIgnoreMoveInput(true);
		}
		return;
	}

	if (AController* OwnerController = OwnerCharacter->GetController())
	{
		OwnerController->SetIgnoreMoveInput(false);
	}

	if (CharacterMovement->MovementMode == MOVE_None)
	{
		CharacterMovement->SetMovementMode(MOVE_Walking);
	}
}

void UHBCrowdControlComponent::EvaluateExpirations(float CurrentServerTime)
{
	if (bIsStunned && CurrentServerTime >= StunEndServerTime)
	{
		bIsStunned = false;
		StunEndServerTime = 0.0f;
		OnRep_IsStunned();
	}

	if (bIsSlowed && CurrentServerTime >= SlowEndServerTime)
	{
		bIsSlowed = false;
		ActiveSlowPercent = 0.0f;
		SlowEndServerTime = 0.0f;
		OnRep_IsSlowed();
		OnRep_ActiveSlowPercent();
	}
}

void UHBCrowdControlComponent::NotifyOwnerCrowdControlChanged() const
{
	if (ABaseHeroCharacter* HeroCharacter = Cast<ABaseHeroCharacter>(GetOwner()))
	{
		HeroCharacter->OnCrowdControlStateChanged();
	}
}

void UHBCrowdControlComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHBCrowdControlComponent, bIsStunned);
	DOREPLIFETIME(UHBCrowdControlComponent, bIsSlowed);
	DOREPLIFETIME(UHBCrowdControlComponent, ActiveSlowPercent);
	DOREPLIFETIME(UHBCrowdControlComponent, StunEndServerTime);
	DOREPLIFETIME(UHBCrowdControlComponent, SlowEndServerTime);
}
