#pragma once

#include "CoreMinimal.h"
#include "source/BaseHeroCharacter.h"
#include "GoalieHero.generated.h"

class AGoalShieldBarrier;
class AHBSoccerBall;
class UAnimMontage;

UCLASS()
class MYPROJECT_API AGoalieHero : public ABaseHeroCharacter
{
	GENERATED_BODY()

public:
	AGoalieHero();

	virtual void ActivateAbility(EHeroAbilitySlot AbilitySlot) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Role")
	float GoalieStartingHealth = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Role")
	float GoalieBaseMovementSpeed = 470.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Dive Save")
	float DiveSaveCooldown = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Dive Save")
	float DiveSaveStaminaCost = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Dive Save")
	float DiveSaveLaunchStrength = 1900.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Dive Save")
	float DiveSaveUpwardStrength = 220.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Goal Shield")
	float GoalShieldCooldown = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Goal Shield")
	float GoalShieldStaminaCost = 40.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Goal Shield")
	float GoalShieldDuration = 2.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Goal Shield")
	float GoalShieldForwardOffset = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Goal Shield")
	TSubclassOf<AGoalShieldBarrier> GoalShieldClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Power Punt")
	float PowerPuntCooldown = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Power Punt")
	float PowerPuntStaminaCost = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Power Punt")
	float PowerPuntImpulseStrength = 5200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Power Punt")
	float PowerPuntBallSearchRadius = 380.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Power Punt")
	float PowerPuntUpwardBlend = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Animations")
	TObjectPtr<UAnimMontage> DiveSaveMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Animations")
	TObjectPtr<UAnimMontage> GoalShieldMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goalie|Animations")
	TObjectPtr<UAnimMontage> PowerPuntMontage = nullptr;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ActiveGoalShield, Category = "Goalie|Goal Shield")
	TObjectPtr<AGoalShieldBarrier> ActiveGoalShield = nullptr;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Goalie|Cooldowns")
	float NextDiveSaveAllowedTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Goalie|Cooldowns")
	float NextGoalShieldAllowedTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Goalie|Cooldowns")
	float NextPowerPuntAllowedTime = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Goalie|Abilities")
	void UseDiveSave(FVector DiveDirection);

	UFUNCTION(BlueprintCallable, Category = "Goalie|Abilities")
	void UseGoalShield();

	UFUNCTION(BlueprintCallable, Category = "Goalie|Abilities")
	void UsePowerPunt();

	UFUNCTION(Server, Reliable)
	void ServerUseDiveSave(FVector_NetQuantizeNormal DiveDirection);

	UFUNCTION(Server, Reliable)
	void ServerUseGoalShield();

	UFUNCTION(Server, Reliable)
	void ServerUsePowerPunt();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastDiveSaveEffect(FVector_NetQuantizeNormal DiveDirection);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastGoalShieldEffect(bool bShieldActivated);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPowerPuntEffect(AHBSoccerBall* PuntedBall);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_ActiveGoalShield();

	UFUNCTION()
	void HandleGoalShieldDestroyed(AActor* DestroyedActor);

	bool IsCooldownReady(float NextAllowedServerTime) const;
	bool SpendStamina(float StaminaCost);
	FVector ResolveDiveDirection(const FVector& RequestedDirection) const;
	AHBSoccerBall* FindNearestBallForPowerPunt() const;
	void PlayAbilityMontage(UAnimMontage* MontageToPlay);
};
