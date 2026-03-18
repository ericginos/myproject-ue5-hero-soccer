#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HBAttributeComponent.generated.h"

UENUM(BlueprintType)
enum class EHBHeroAttribute : uint8
{
	Health UMETA(DisplayName = "Health"),
	Stamina UMETA(DisplayName = "Stamina"),
	MovementSpeed UMETA(DisplayName = "Movement Speed"),
	KickPower UMETA(DisplayName = "Kick Power"),
	PassAccuracy UMETA(DisplayName = "Pass Accuracy"),
	AbilityCooldownReduction UMETA(DisplayName = "Ability Cooldown Reduction")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHeroAttributeChanged, EHBHeroAttribute, Attribute, float, OldValue, float, NewValue);

UCLASS(ClassGroup=(Custom), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class MYPROJECT_API UHBAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHBAttributeComponent();

	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FOnHeroAttributeChanged OnHeroAttributeChanged;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Attributes")
	float Health = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "Attributes")
	float Stamina = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_MovementSpeed, Category = "Attributes")
	float MovementSpeed = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_KickPower, Category = "Attributes")
	float KickPower = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_PassAccuracy, Category = "Attributes")
	float PassAccuracy = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_AbilityCooldownReduction, Category = "Attributes")
	float AbilityCooldownReduction = 0.0f;

	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetAttributeValue(EHBHeroAttribute Attribute) const;

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	bool ModifyAttribute(EHBHeroAttribute Attribute, float Delta);

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	bool SetAttributeValue(EHBHeroAttribute Attribute, float NewValue);

	UFUNCTION(Server, Reliable)
	void Server_RequestModifyAttribute(EHBHeroAttribute Attribute, float Delta);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Health(float OldValue);

	UFUNCTION()
	void OnRep_Stamina(float OldValue);

	UFUNCTION()
	void OnRep_MovementSpeed(float OldValue);

	UFUNCTION()
	void OnRep_KickPower(float OldValue);

	UFUNCTION()
	void OnRep_PassAccuracy(float OldValue);

	UFUNCTION()
	void OnRep_AbilityCooldownReduction(float OldValue);

	void BroadcastChange(EHBHeroAttribute Attribute, float OldValue, float NewValue);
	bool SetAttributeValueInternal(EHBHeroAttribute Attribute, float NewValue);
	float SanitizeAttributeValue(EHBHeroAttribute Attribute, float InValue) const;
	void ApplyMovementSpeedToOwner() const;
};
