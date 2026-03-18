#include "source/BaseHeroCharacter.h"

#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "source/HBAbilityComponent.h"
#include "source/HBAttributeComponent.h"
#include "source/HBCrowdControlComponent.h"
#include "source/HBSoccerBall.h"
#include "source/HBSoccerPlayerState.h"
#include "GameFramework/SpringArmComponent.h"

ABaseHeroCharacter::ABaseHeroCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(60.0f);
	SetMinNetUpdateFrequency(20.0f);
	NetPriority = 2.2f;
	SetNetCullDistanceSquared(FMath::Square(20000.0f));
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	AttributeComponent = CreateDefaultSubobject<UHBAttributeComponent>(TEXT("AttributeComponent"));
	AbilityComponent = CreateDefaultSubobject<UHBAbilityComponent>(TEXT("AbilityComponent"));
	CrowdControlComponent = CreateDefaultSubobject<UHBCrowdControlComponent>(TEXT("CrowdControlComponent"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 420.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.0f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->FieldOfView = BaseCameraFOV;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}
}

void ABaseHeroCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (FollowCamera)
	{
		FollowCamera->SetFieldOfView(BaseCameraFOV);
	}

	UpdateSprintSpeed();
}

void ABaseHeroCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority())
	{
		ProcessStamina(DeltaSeconds);
	}

	UpdateCompetitiveCamera(DeltaSeconds);
}

void ABaseHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!PlayerInputComponent)
	{
		return;
	}

	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &ABaseHeroCharacter::InputStartSprint);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &ABaseHeroCharacter::InputStopSprint);
	PlayerInputComponent->BindAction(TEXT("AbilityOne"), IE_Pressed, this, &ABaseHeroCharacter::InputActivateAbilityOne);
	PlayerInputComponent->BindAction(TEXT("AbilityTwo"), IE_Pressed, this, &ABaseHeroCharacter::InputActivateAbilityTwo);
	PlayerInputComponent->BindAction(TEXT("AbilityThree"), IE_Pressed, this, &ABaseHeroCharacter::InputActivateAbilityThree);
}

void ABaseHeroCharacter::StartSprinting()
{
	if (!HasAuthority())
	{
		ServerSetSprinting(true);
		return;
	}

	if (!HasStaminaForSprint())
	{
		return;
	}

	if (CrowdControlComponent && CrowdControlComponent->IsStunned())
	{
		return;
	}

	if (bIsExhausted)
	{
		return;
	}

	bIsSprinting = true;
	OnRep_IsSprinting();
}

void ABaseHeroCharacter::StopSprinting()
{
	if (!HasAuthority())
	{
		ServerSetSprinting(false);
		return;
	}

	bIsSprinting = false;
	OnRep_IsSprinting();
}

void ABaseHeroCharacter::ActivateAbility(EHeroAbilitySlot AbilitySlot)
{
	if (CrowdControlComponent && CrowdControlComponent->IsStunned())
	{
		return;
	}

	if (!AbilityComponent)
	{
		return;
	}

	AbilityComponent->TryActivateAbility(AbilitySlot);
}

void ABaseHeroCharacter::OnCrowdControlStateChanged()
{
	if (HasAuthority() && CrowdControlComponent && CrowdControlComponent->IsStunned() && bIsSprinting)
	{
		bIsSprinting = false;
		OnRep_IsSprinting();
	}

	UpdateSprintSpeed();
}

ABaseHeroCharacter* ABaseHeroCharacter::GetSoftLockPassTarget() const
{
	if (CachedSoftLockPassTarget.IsValid())
	{
		return CachedSoftLockPassTarget.Get();
	}

	return FindBestSoftLockPassTarget();
}

ABaseHeroCharacter* ABaseHeroCharacter::FindBestSoftLockPassTarget() const
{
	if (!bEnableSoftLockPassTargeting || !GetWorld())
	{
		return nullptr;
	}

	const ETeam MyTeam = ResolveHeroTeam();
	if (MyTeam == ETeam::None)
	{
		return nullptr;
	}

	const float MaxRange = FMath::Max(1.0f, SoftLockPassRange);
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(SoftLockPassConeHalfAngleDegrees, 1.0f, 89.0f)));
	const FVector ForwardDirection = GetControlRotation().Vector().GetSafeNormal2D().IsNearlyZero() ?
		GetActorForwardVector().GetSafeNormal2D() :
		GetControlRotation().Vector().GetSafeNormal2D();

	ABaseHeroCharacter* BestTarget = nullptr;
	float BestScore = TNumericLimits<float>::Lowest();

	for (TActorIterator<ABaseHeroCharacter> It(GetWorld()); It; ++It)
	{
		ABaseHeroCharacter* Candidate = *It;
		if (!Candidate || Candidate == this)
		{
			continue;
		}

		if (Candidate->ResolveHeroTeam() != MyTeam)
		{
			continue;
		}

		const FVector ToCandidate = Candidate->GetActorLocation() - GetActorLocation();
		const float Distance = ToCandidate.Size();
		if (Distance <= KINDA_SMALL_NUMBER || Distance > MaxRange)
		{
			continue;
		}

		const FVector ToCandidateDirection = ToCandidate / Distance;
		const float ForwardDot = FVector::DotProduct(ForwardDirection, ToCandidateDirection.GetSafeNormal2D());
		if (ForwardDot < MinDot)
		{
			continue;
		}

		const float DistanceScore = 1.0f - FMath::Clamp(Distance / MaxRange, 0.0f, 1.0f);
		float Score =
			(ForwardDot * FMath::Clamp(SoftLockTargetDotWeight, 0.0f, 1.0f)) +
			(DistanceScore * FMath::Clamp(SoftLockTargetDistanceWeight, 0.0f, 1.0f));

		if (Candidate == CachedSoftLockPassTarget.Get())
		{
			Score += 0.1f;
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

float ABaseHeroCharacter::ApplyHeroDamage(float RawDamage)
{
	if (!HasAuthority() || !AttributeComponent)
	{
		return 0.0f;
	}

	const float PositiveDamage = FMath::Max(0.0f, RawDamage);
	if (PositiveDamage <= 0.0f)
	{
		return 0.0f;
	}

	const float FinalDamage = PositiveDamage * FMath::Clamp(IncomingDamageMultiplier, 0.05f, 1.0f);
	AttributeComponent->ModifyAttribute(EHBHeroAttribute::Health, -FinalDamage);
	return FinalDamage;
}

void ABaseHeroCharacter::SetIncomingDamageMultiplier(float NewMultiplier)
{
	if (!HasAuthority())
	{
		return;
	}

	IncomingDamageMultiplier = FMath::Clamp(NewMultiplier, 0.05f, 1.0f);
	OnRep_IncomingDamageMultiplier();
}

void ABaseHeroCharacter::SetExternalSpeedMultiplier(float NewMultiplier)
{
	if (!HasAuthority())
	{
		return;
	}

	ExternalSpeedMultiplier = FMath::Clamp(NewMultiplier, 0.1f, 3.0f);
	OnRep_ExternalSpeedMultiplier();
}

void ABaseHeroCharacter::ServerSetSprinting_Implementation(bool bNewSprinting)
{
	const AController* HeroController = GetController();
	if (!HeroController || !HeroController->IsPlayerController())
	{
		return;
	}

	if (bNewSprinting)
	{
		StartSprinting();
		return;
	}

	StopSprinting();
}

void ABaseHeroCharacter::OnRep_IsSprinting()
{
	UpdateSprintSpeed();
}

void ABaseHeroCharacter::OnRep_IsExhausted()
{
	UpdateSprintSpeed();
}

void ABaseHeroCharacter::OnRep_IncomingDamageMultiplier()
{
	// Hook for local VFX/UI reaction to mitigation changes.
}

void ABaseHeroCharacter::OnRep_ExternalSpeedMultiplier()
{
	UpdateSprintSpeed();
}

void ABaseHeroCharacter::UpdateSprintSpeed()
{
	if (!GetCharacterMovement() || !AttributeComponent)
	{
		return;
	}

	const float BaseSpeed = AttributeComponent->GetAttributeValue(EHBHeroAttribute::MovementSpeed);
	float NewSpeed = BaseSpeed * ExternalSpeedMultiplier;

	if (CrowdControlComponent)
	{
		NewSpeed *= CrowdControlComponent->GetMovementSpeedMultiplier();
	}

	if (NewSpeed <= 0.0f)
	{
		GetCharacterMovement()->MaxWalkSpeed = 0.0f;
		return;
	}

	if (bIsExhausted)
	{
		NewSpeed *= FMath::Clamp(ExhaustedSpeedMultiplier, 0.1f, 1.0f);
	}
	else if (bIsSprinting)
	{
		NewSpeed *= SprintSpeedMultiplier;
	}

	GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
}

void ABaseHeroCharacter::UpdateCompetitiveCamera(float DeltaSeconds)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	if (FollowCamera)
	{
		const float TargetFOV = bIsSprinting ? SprintCameraFOV : BaseCameraFOV;
		const float InterpSpeed = FMath::Max(0.0f, CameraFOVInterpSpeed);
		const float NewFOV = (InterpSpeed <= KINDA_SMALL_NUMBER)
			? TargetFOV
			: FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaSeconds, InterpSpeed);
		FollowCamera->SetFieldOfView(NewFOV);
	}

	CachedSoftLockPassTarget = FindBestSoftLockPassTarget();

	if (!bEnableBallTrackingPriority)
	{
		return;
	}

	AController* OwnerController = GetController();
	AHBSoccerBall* PriorityBall = FindPrioritySoccerBallForCamera();
	if (!OwnerController || !PriorityBall)
	{
		return;
	}

	const FVector ViewLocation = GetPawnViewLocation();
	const FVector ToBallDirection = (PriorityBall->GetActorLocation() - ViewLocation).GetSafeNormal();
	if (ToBallDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator CurrentControlRotation = OwnerController->GetControlRotation();
	const FVector CurrentForward = CurrentControlRotation.Vector();
	const FVector DesiredForward = FMath::Lerp(CurrentForward, ToBallDirection, FMath::Clamp(BallTrackingBlendWeight, 0.0f, 1.0f)).GetSafeNormal();
	FRotator DesiredRotation = DesiredForward.Rotation();
	DesiredRotation.Pitch = FMath::ClampAngle(DesiredRotation.Pitch, -70.0f, 40.0f);

	const float TrackingInterpSpeed = FMath::Max(0.0f, BallTrackingRotationInterpSpeed);
	const FRotator NewControlRotation = (TrackingInterpSpeed <= KINDA_SMALL_NUMBER)
		? DesiredRotation
		: FMath::RInterpTo(CurrentControlRotation, DesiredRotation, DeltaSeconds, TrackingInterpSpeed);

	OwnerController->SetControlRotation(NewControlRotation);
}

AHBSoccerBall* ABaseHeroCharacter::FindPrioritySoccerBallForCamera() const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	AHBSoccerBall* BestBall = nullptr;
	float BestScore = TNumericLimits<float>::Lowest();
	const float MaxDistance = FMath::Max(1.0f, BallTrackingPriorityDistance);
	const float MaxDistanceSq = FMath::Square(MaxDistance);

	for (TActorIterator<AHBSoccerBall> It(GetWorld()); It; ++It)
	{
		AHBSoccerBall* Ball = *It;
		if (!Ball)
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(GetActorLocation(), Ball->GetActorLocation());
		if (DistanceSq > MaxDistanceSq)
		{
			continue;
		}

		const float DistanceAlpha = 1.0f - FMath::Clamp(FMath::Sqrt(DistanceSq) / MaxDistance, 0.0f, 1.0f);
		float Score = DistanceAlpha * 100.0f;

		if (Ball->GetPossessingPawn() == this)
		{
			Score += 500.0f;
		}
		else if (Ball->GetPossessingPawn())
		{
			Score += 80.0f;
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			BestBall = Ball;
		}
	}

	return BestBall;
}

ETeam ABaseHeroCharacter::ResolveHeroTeam() const
{
	const AHBSoccerPlayerState* SoccerState = GetPlayerState<AHBSoccerPlayerState>();
	return SoccerState ? SoccerState->Team : ETeam::None;
}

bool ABaseHeroCharacter::HasStaminaForSprint() const
{
	if (!AttributeComponent)
	{
		return false;
	}

	const float CurrentStamina = AttributeComponent->GetAttributeValue(EHBHeroAttribute::Stamina);
	return !bIsExhausted && CurrentStamina > 0.0f;
}

void ABaseHeroCharacter::ProcessStamina(float DeltaSeconds)
{
	if (!AttributeComponent)
	{
		return;
	}

	if (bIsSprinting)
	{
		const float DrainAmount = FMath::Max(0.0f, StaminaDrainPerSecond * DeltaSeconds);
		AttributeComponent->ModifyAttribute(EHBHeroAttribute::Stamina, -DrainAmount);

		const float CurrentStamina = AttributeComponent->GetAttributeValue(EHBHeroAttribute::Stamina);
		if (CurrentStamina <= 0.0f)
		{
			bIsExhausted = true;
			bIsSprinting = false;
			OnRep_IsExhausted();
			OnRep_IsSprinting();
		}
		return;
	}

	const float RecoverAmount = FMath::Max(0.0f, StaminaRecoveryPerSecond * DeltaSeconds);
	AttributeComponent->ModifyAttribute(EHBHeroAttribute::Stamina, RecoverAmount);

	const float CurrentStamina = AttributeComponent->GetAttributeValue(EHBHeroAttribute::Stamina);
	if (bIsExhausted && CurrentStamina >= FMath::Max(0.0f, StaminaRecoveryThresholdToSprint))
	{
		bIsExhausted = false;
		OnRep_IsExhausted();
	}
	else if (!bIsExhausted && CurrentStamina <= 0.0f)
	{
		bIsExhausted = true;
		OnRep_IsExhausted();
	}
}

void ABaseHeroCharacter::InputStartSprint()
{
	StartSprinting();
}

void ABaseHeroCharacter::InputStopSprint()
{
	StopSprinting();
}

void ABaseHeroCharacter::InputActivateAbilityOne()
{
	ActivateAbility(EHeroAbilitySlot::AbilityOne);
}

void ABaseHeroCharacter::InputActivateAbilityTwo()
{
	ActivateAbility(EHeroAbilitySlot::AbilityTwo);
}

void ABaseHeroCharacter::InputActivateAbilityThree()
{
	ActivateAbility(EHeroAbilitySlot::AbilityThree);
}

void ABaseHeroCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseHeroCharacter, bIsSprinting);
	DOREPLIFETIME(ABaseHeroCharacter, bIsExhausted);
	DOREPLIFETIME(ABaseHeroCharacter, IncomingDamageMultiplier);
	DOREPLIFETIME(ABaseHeroCharacter, ExternalSpeedMultiplier);
}
