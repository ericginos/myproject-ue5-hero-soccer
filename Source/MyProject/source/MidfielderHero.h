#pragma once

#include "CoreMinimal.h"
#include "source/BaseHeroCharacter.h"
#include "source/SoccerTypes.h"
#include "MidfielderHero.generated.h"

class ABaseHeroCharacter;
class AHBSoccerBall;
class APawn;
class UAnimMontage;

UCLASS()
class MYPROJECT_API AMidfielderHero : public ABaseHeroCharacter
{
	GENERATED_BODY()

public:
	AMidfielderHero();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void ActivateAbility(EHeroAbilitySlot AbilitySlot) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Role")
	float MidfielderStartingHealth = 130.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Role")
	float MidfielderBaseMovementSpeed = 620.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Speed Aura")
	float SpeedBoostAuraCooldown = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Speed Aura")
	float SpeedBoostAuraStaminaCost = 26.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Speed Aura")
	float SpeedBoostAuraDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Speed Aura")
	float SpeedBoostAuraRadius = 900.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Speed Aura")
	float SpeedBoostAuraMultiplier = 1.22f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Precision Pass")
	float PrecisionPassCooldown = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Precision Pass")
	float PrecisionPassStaminaCost = 16.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Precision Pass")
	float PrecisionPassImpulseStrength = 3200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Precision Pass")
	float PrecisionPassBallSearchRadius = 360.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Precision Pass")
	float PrecisionPassTargetRange = 3200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Precision Pass", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PrecisionPassAssistBlend = 0.85f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Stamina Field")
	float StaminaRegenFieldCooldown = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Stamina Field")
	float StaminaRegenFieldStaminaCost = 24.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Stamina Field")
	float StaminaRegenFieldDuration = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Stamina Field")
	float StaminaRegenFieldRadius = 850.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Stamina Field")
	float StaminaRegenPerSecond = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Animation")
	TObjectPtr<UAnimMontage> SpeedBoostAuraMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Animation")
	TObjectPtr<UAnimMontage> PrecisionPassMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Midfielder|Animation")
	TObjectPtr<UAnimMontage> StaminaRegenFieldMontage = nullptr;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SpeedBoostAuraActive, Category = "Midfielder|Speed Aura")
	bool bSpeedBoostAuraActive = false;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_StaminaRegenFieldActive, Category = "Midfielder|Stamina Field")
	bool bStaminaRegenFieldActive = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Midfielder|Cooldowns")
	float NextSpeedBoostAuraAllowedTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Midfielder|Cooldowns")
	float NextPrecisionPassAllowedTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Midfielder|Cooldowns")
	float NextStaminaRegenFieldAllowedTime = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Midfielder|Abilities")
	void UseSpeedBoostAura();

	UFUNCTION(BlueprintCallable, Category = "Midfielder|Abilities")
	void UsePrecisionPass();

	UFUNCTION(BlueprintCallable, Category = "Midfielder|Abilities")
	void UseStaminaRegenField();

	UFUNCTION(Server, Reliable)
	void ServerUseSpeedBoostAura();

	UFUNCTION(Server, Reliable)
	void ServerUsePrecisionPass();

	UFUNCTION(Server, Reliable)
	void ServerUseStaminaRegenField();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastSpeedBoostAuraEffect(bool bAuraStarted);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPrecisionPassEffect(AHBSoccerBall* PassedBall, ABaseHeroCharacter* TargetTeammate);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastStaminaRegenFieldEffect(bool bFieldStarted);

private:
	UPROPERTY()
	TSet<TWeakObjectPtr<ABaseHeroCharacter>> SpeedAuraBuffedAllies;

	float SpeedBoostAuraEndServerTime = 0.0f;
	float StaminaRegenFieldEndServerTime = 0.0f;

	UFUNCTION()
	void OnRep_SpeedBoostAuraActive();

	UFUNCTION()
	void OnRep_StaminaRegenFieldActive();

	bool IsCooldownReady(float NextAllowedServerTime) const;
	bool SpendStamina(float StaminaCost);
	ETeam GetTeamForPawn(const APawn* Pawn) const;
	void UpdateSpeedAuraRecipients();
	void ClearSpeedAuraRecipients();
	void EndSpeedBoostAura();
	void ProcessStaminaRegenField(float DeltaSeconds);
	void EndStaminaRegenField();
	AHBSoccerBall* FindNearestBallForPrecisionPass() const;
	ABaseHeroCharacter* FindBestTeammateForPrecisionPass() const;
	void PlayAbilityMontage(UAnimMontage* MontageToPlay);
};
