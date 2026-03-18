#pragma once

#include "CoreMinimal.h"
#include "source/BaseHeroCharacter.h"
#include "source/SoccerTypes.h"
#include "DefenderHero.generated.h"

class ABaseHeroCharacter;
class APawn;
class UAnimMontage;

UCLASS()
class MYPROJECT_API ADefenderHero : public ABaseHeroCharacter
{
	GENERATED_BODY()

public:
	ADefenderHero();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void ActivateAbility(EHeroAbilitySlot AbilitySlot) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Role")
	float DefenderStartingHealth = 240.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Role")
	float DefenderBaseMovementSpeed = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Body Check")
	float BodyCheckCooldown = 2.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Body Check")
	float BodyCheckStaminaCost = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Body Check")
	float BodyCheckRange = 280.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Body Check")
	float BodyCheckKnockbackStrength = 1800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Body Check")
	float BodyCheckUpwardStrength = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Intercept Dash")
	float InterceptDashCooldown = 4.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Intercept Dash")
	float InterceptDashStaminaCost = 24.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Intercept Dash")
	float InterceptDashStrength = 2200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Intercept Dash")
	float InterceptDashUpwardStrength = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Defensive Aura")
	float DefensiveAuraCooldown = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Defensive Aura")
	float DefensiveAuraStaminaCost = 32.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Defensive Aura")
	float DefensiveAuraDuration = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Defensive Aura")
	float DefensiveAuraRadius = 900.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Defensive Aura")
	float DefensiveAuraDamageMultiplier = 0.70f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Animation")
	TObjectPtr<UAnimMontage> BodyCheckMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Animation")
	TObjectPtr<UAnimMontage> InterceptDashMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defender|Animation")
	TObjectPtr<UAnimMontage> DefensiveAuraMontage = nullptr;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DefensiveAuraActive, Category = "Defender|Defensive Aura")
	bool bDefensiveAuraActive = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Defender|Cooldowns")
	float NextBodyCheckAllowedTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Defender|Cooldowns")
	float NextInterceptDashAllowedTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Defender|Cooldowns")
	float NextDefensiveAuraAllowedTime = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Defender|Abilities")
	void UseBodyCheck();

	UFUNCTION(BlueprintCallable, Category = "Defender|Abilities")
	void UseInterceptDash(FVector DashDirection);

	UFUNCTION(BlueprintCallable, Category = "Defender|Abilities")
	void UseDefensiveAura();

	UFUNCTION(Server, Reliable)
	void ServerUseBodyCheck();

	UFUNCTION(Server, Reliable)
	void ServerUseInterceptDash(FVector_NetQuantizeNormal DashDirection);

	UFUNCTION(Server, Reliable)
	void ServerUseDefensiveAura();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastBodyCheckEffect();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastInterceptDashEffect(FVector_NetQuantizeNormal DashDirection);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastDefensiveAuraEffect(bool bAuraActivated);

private:
	UPROPERTY()
	TSet<TWeakObjectPtr<ABaseHeroCharacter>> AuraBuffedAllies;

	float DefensiveAuraEndServerTime = 0.0f;

	UFUNCTION()
	void OnRep_DefensiveAuraActive();

	bool IsCooldownReady(float NextAllowedServerTime) const;
	bool SpendStamina(float StaminaCost);
	FVector ResolveDashDirection(const FVector& RequestedDirection) const;
	ETeam GetTeamForPawn(const APawn* Pawn) const;
	void UpdateDefensiveAuraRecipients();
	void EndDefensiveAura();
	void ClearDefensiveAuraBuffs();
	void PlayAbilityMontage(UAnimMontage* MontageToPlay);
};
