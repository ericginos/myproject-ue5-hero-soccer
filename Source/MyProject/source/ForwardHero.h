#pragma once

#include "CoreMinimal.h"
#include "source/BaseHeroCharacter.h"
#include "ForwardHero.generated.h"

class AHBSoccerBall;
class UAnimMontage;

UCLASS()
class MYPROJECT_API AForwardHero : public ABaseHeroCharacter
{
	GENERATED_BODY()

public:
	AForwardHero();

	virtual void BeginPlay() override;
	virtual void ActivateAbility(EHeroAbilitySlot AbilitySlot) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Role")
	float ForwardStartingHealth = 155.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Role")
	float ForwardBaseMovementSpeed = 610.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Role")
	float ForwardKickPower = 2.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Power Shot")
	float PowerShotCooldown = 2.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Power Shot")
	float PowerShotStaminaCost = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Power Shot")
	float PowerShotImpulseStrength = 6200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Power Shot")
	float PowerShotBallSearchRadius = 420.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Power Shot")
	float PowerShotUpwardBlend = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Curve Shot")
	float CurveShotCooldown = 3.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Curve Shot")
	float CurveShotStaminaCost = 24.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Curve Shot")
	float CurveShotImpulseStrength = 5200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Curve Shot")
	float CurveShotBallSearchRadius = 420.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Curve Shot")
	float CurveShotSideBlend = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Curve Shot")
	float CurveShotUpwardBlend = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Breakaway Dash")
	float BreakawayDashCooldown = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Breakaway Dash")
	float BreakawayDashStaminaCost = 18.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Breakaway Dash")
	float BreakawayDashStrength = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Breakaway Dash")
	float BreakawayDashUpwardStrength = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Animation")
	TObjectPtr<UAnimMontage> PowerShotMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Animation")
	TObjectPtr<UAnimMontage> CurveShotMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forward|Animation")
	TObjectPtr<UAnimMontage> BreakawayDashMontage = nullptr;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Forward|Cooldowns")
	float NextPowerShotAllowedTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Forward|Cooldowns")
	float NextCurveShotAllowedTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Forward|Cooldowns")
	float NextBreakawayDashAllowedTime = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Forward|Abilities")
	void UsePowerShot();

	UFUNCTION(BlueprintCallable, Category = "Forward|Abilities")
	void UseCurveShot();

	UFUNCTION(BlueprintCallable, Category = "Forward|Abilities")
	void UseBreakawayDash(FVector DashDirection);

	UFUNCTION(Server, Reliable)
	void ServerUsePowerShot();

	UFUNCTION(Server, Reliable)
	void ServerUseCurveShot();

	UFUNCTION(Server, Reliable)
	void ServerUseBreakawayDash(FVector_NetQuantizeNormal DashDirection);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPowerShotEffect(AHBSoccerBall* ShotBall);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastCurveShotEffect(AHBSoccerBall* ShotBall);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastBreakawayDashEffect(FVector_NetQuantizeNormal DashDirection);

private:
	bool IsCooldownReady(float NextAllowedServerTime) const;
	bool SpendStamina(float StaminaCost);
	float GetKickPowerMultiplier() const;
	AHBSoccerBall* FindNearestBall(float SearchRadius) const;
	FVector BuildShotDirection(float SideBlend, float UpwardBlend) const;
	FVector ResolveDashDirection(const FVector& RequestedDirection) const;
	void PlayAbilityMontage(UAnimMontage* MontageToPlay);
};
