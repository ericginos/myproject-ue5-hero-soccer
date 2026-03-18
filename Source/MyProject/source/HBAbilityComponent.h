#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HBAbilityComponent.generated.h"

class APawn;
class UAnimMontage;
class UHBAttributeComponent;

UENUM(BlueprintType)
enum class EHeroAbilitySlot : uint8
{
	AbilityOne UMETA(DisplayName = "Ability One"),
	AbilityTwo UMETA(DisplayName = "Ability Two"),
	AbilityThree UMETA(DisplayName = "Ability Three")
};

UENUM(BlueprintType)
enum class EHeroAbilityType : uint8
{
	None UMETA(DisplayName = "None"),
	DashBoost UMETA(DisplayName = "Dash Boost")
};

USTRUCT(BlueprintType)
struct FHeroAbilitySpec
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	EHeroAbilityType AbilityType = EHeroAbilityType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	float CooldownSeconds = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	float StaminaCost = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TObjectPtr<UAnimMontage> ActivationMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DashBoost", meta = (ClampMin = "0.0"))
	float DashStrength = 1500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DashBoost")
	float DashUpwardStrength = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHeroAbilityActivated, EHeroAbilitySlot, AbilitySlot, APawn*, ActivatingPawn);

USTRUCT(BlueprintType)
struct FReplicatedAbilityActivation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Ability")
	EHeroAbilitySlot AbilitySlot = EHeroAbilitySlot::AbilityOne;

	UPROPERTY(BlueprintReadOnly, Category = "Ability")
	int32 ActivationId = 0;
};

UCLASS(ClassGroup=(Custom), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class MYPROJECT_API UHBAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHBAbilityComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	FHeroAbilitySpec AbilityOneSpec;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	FHeroAbilitySpec AbilityTwoSpec;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	FHeroAbilitySpec AbilityThreeSpec;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ReplicatedActivation, Category = "Ability")
	FReplicatedAbilityActivation ReplicatedActivation;

	UPROPERTY(BlueprintAssignable, Category = "Ability")
	FOnHeroAbilityActivated OnHeroAbilityActivated;

	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool TryActivateAbility(EHeroAbilitySlot AbilitySlot);

	UFUNCTION(BlueprintPure, Category = "Ability")
	bool IsAbilityOnCooldown(EHeroAbilitySlot AbilitySlot) const;

	UFUNCTION(BlueprintPure, Category = "Ability")
	float GetRemainingCooldown(EHeroAbilitySlot AbilitySlot) const;

	UFUNCTION(Server, Reliable)
	void ServerTryActivateAbility(EHeroAbilitySlot AbilitySlot);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayAbilityEffect(EHeroAbilitySlot AbilitySlot, APawn* ActivatingPawn, int32 ActivationId);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(Replicated, meta = (AllowPrivateAccess = "true"))
	float AbilityOneCooldownEndTime = 0.0f;

	UPROPERTY(Replicated, meta = (AllowPrivateAccess = "true"))
	float AbilityTwoCooldownEndTime = 0.0f;

	UPROPERTY(Replicated, meta = (AllowPrivateAccess = "true"))
	float AbilityThreeCooldownEndTime = 0.0f;

	int32 LastProcessedActivationId = 0;

	UFUNCTION()
	void OnRep_ReplicatedActivation();

	const FHeroAbilitySpec& GetSpec(EHeroAbilitySlot AbilitySlot) const;
	float GetCooldownEndTime(EHeroAbilitySlot AbilitySlot) const;
	void SetCooldownEndTime(EHeroAbilitySlot AbilitySlot, float NewCooldownEndTime);
	bool CommitAbilityOnServer(EHeroAbilitySlot AbilitySlot);
	bool ExecuteAbilityOnServer(EHeroAbilitySlot AbilitySlot, APawn* OwnerPawn);
	void PlayAbilityMontage(APawn* ActivatingPawn, EHeroAbilitySlot AbilitySlot) const;
	UHBAttributeComponent* GetAttributeComponent() const;
};
