#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "source/HBAbilityComponent.h"
#include "source/SoccerTypes.h"
#include "BaseHeroCharacter.generated.h"

class AHBSoccerBall;
class UHBAbilityComponent;
class UHBAttributeComponent;
class UHBCrowdControlComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class MYPROJECT_API ABaseHeroCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABaseHeroCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHBAttributeComponent> AttributeComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHBAbilityComponent> AbilityComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHBCrowdControlComponent> CrowdControlComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Camera")
	TObjectPtr<UCameraComponent> FollowCamera = nullptr;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsSprinting, Category = "Movement")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsExhausted, Category = "Stamina")
	bool bIsExhausted = false;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IncomingDamageMultiplier, Category = "Combat")
	float IncomingDamageMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ExternalSpeedMultiplier, Category = "Movement")
	float ExternalSpeedMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float SprintSpeedMultiplier = 1.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float BaseCameraFOV = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float SprintCameraFOV = 103.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float CameraFOVInterpSpeed = 7.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	bool bEnableBallTrackingPriority = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float BallTrackingPriorityDistance = 2600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float BallTrackingBlendWeight = 0.28f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float BallTrackingRotationInterpSpeed = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina")
	float StaminaDrainPerSecond = 22.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina")
	float StaminaRecoveryPerSecond = 14.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina")
	float ExhaustedSpeedMultiplier = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina")
	float StaminaRecoveryThresholdToSprint = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	bool bEnableSoftLockPassTargeting = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	float SoftLockPassRange = 3000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	float SoftLockPassConeHalfAngleDegrees = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	float SoftLockTargetDotWeight = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	float SoftLockTargetDistanceWeight = 0.25f;

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StartSprinting();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StopSprinting();

	UFUNCTION(BlueprintCallable, Category = "Ability")
	virtual void ActivateAbility(EHeroAbilitySlot AbilitySlot);

	UFUNCTION(BlueprintCallable, Category = "Crowd Control")
	void OnCrowdControlStateChanged();

	UFUNCTION(BlueprintPure, Category = "Targeting")
	ABaseHeroCharacter* GetSoftLockPassTarget() const;

	UFUNCTION(BlueprintPure, Category = "Targeting")
	ABaseHeroCharacter* FindBestSoftLockPassTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	float ApplyHeroDamage(float RawDamage);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetIncomingDamageMultiplier(float NewMultiplier);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetExternalSpeedMultiplier(float NewMultiplier);

	UFUNCTION(BlueprintPure, Category = "Movement")
	float GetExternalSpeedMultiplier() const { return ExternalSpeedMultiplier; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetIncomingDamageMultiplier() const { return IncomingDamageMultiplier; }

	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(bool bNewSprinting);

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<ABaseHeroCharacter> CachedSoftLockPassTarget;

	UFUNCTION()
	void OnRep_IsSprinting();

	UFUNCTION()
	void OnRep_IsExhausted();

	UFUNCTION()
	void OnRep_IncomingDamageMultiplier();

	UFUNCTION()
	void OnRep_ExternalSpeedMultiplier();

	void UpdateSprintSpeed();
	void UpdateCompetitiveCamera(float DeltaSeconds);
	AHBSoccerBall* FindPrioritySoccerBallForCamera() const;
	ETeam ResolveHeroTeam() const;
	bool HasStaminaForSprint() const;
	void ProcessStamina(float DeltaSeconds);

	void InputStartSprint();
	void InputStopSprint();
	void InputActivateAbilityOne();
	void InputActivateAbilityTwo();
	void InputActivateAbilityThree();
};
