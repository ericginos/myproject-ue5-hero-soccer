#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HBCrowdControlComponent.generated.h"

UCLASS(ClassGroup=(Custom), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class MYPROJECT_API UHBCrowdControlComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHBCrowdControlComponent();

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsStunned, Category = "Crowd Control")
	bool bIsStunned = false;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsSlowed, Category = "Crowd Control")
	bool bIsSlowed = false;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ActiveSlowPercent, Category = "Crowd Control")
	float ActiveSlowPercent = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Crowd Control")
	float StunEndServerTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Crowd Control")
	float SlowEndServerTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crowd Control|Anti-Abuse")
	float KnockbackReapplyCooldown = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crowd Control|Anti-Abuse")
	float SlowReapplyCooldown = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crowd Control|Anti-Abuse")
	float StunReapplyCooldown = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crowd Control|Limits")
	float MaxKnockbackStrength = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crowd Control|Limits")
	float MaxSlowPercent = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crowd Control|Limits")
	float MaxSlowDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crowd Control|Limits")
	float MaxStunDuration = 3.0f;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Crowd Control")
	bool ApplyKnockback(FVector KnockbackDirection, float Strength);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Crowd Control")
	bool ApplySlow(float SlowPercent, float DurationSeconds);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Crowd Control")
	bool ApplyStun(float DurationSeconds);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Crowd Control")
	void ClearAllCrowdControl();

	UFUNCTION(BlueprintPure, Category = "Crowd Control")
	bool IsStunned() const { return bIsStunned; }

	UFUNCTION(BlueprintPure, Category = "Crowd Control")
	bool IsSlowed() const { return bIsSlowed; }

	UFUNCTION(BlueprintPure, Category = "Crowd Control")
	float GetActiveSlowPercent() const { return ActiveSlowPercent; }

	UFUNCTION(BlueprintPure, Category = "Crowd Control")
	float GetMovementSpeedMultiplier() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	float LastKnockbackApplyServerTime = -1000000.0f;
	float LastSlowApplyServerTime = -1000000.0f;
	float LastStunApplyServerTime = -1000000.0f;

	UFUNCTION()
	void OnRep_IsStunned();

	UFUNCTION()
	void OnRep_IsSlowed();

	UFUNCTION()
	void OnRep_ActiveSlowPercent();

	void ApplyStunState(bool bShouldStun) const;
	void EvaluateExpirations(float CurrentServerTime);
	void NotifyOwnerCrowdControlChanged() const;
};
