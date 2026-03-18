#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "source/HBAbilityComponent.h"
#include "source/HBAttributeComponent.h"
#include "HBSoccerHUD.generated.h"

class AHBSoccerGameState;
class APawn;
class UHBAbilityComponent;
class UHBAttributeComponent;

UCLASS()
class MYPROJECT_API AHBSoccerHUD : public AHUD
{
	GENERATED_BODY()

public:
	AHBSoccerHUD();

	virtual void BeginPlay() override;
	virtual void DrawHUD() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Vitals")
	float Health = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Vitals")
	float Stamina = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Abilities")
	float AbilityOneCooldownRemaining = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Abilities")
	float AbilityTwoCooldownRemaining = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Abilities")
	float AbilityThreeCooldownRemaining = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Match")
	int32 TeamAScore = 0;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Match")
	int32 TeamBScore = 0;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Match")
	int32 MatchTimeSeconds = 0;

private:
	TWeakObjectPtr<AHBSoccerGameState> CachedGameState;
	TWeakObjectPtr<APawn> CachedPawn;
	TWeakObjectPtr<UHBAttributeComponent> CachedAttributeComponent;
	TWeakObjectPtr<UHBAbilityComponent> CachedAbilityComponent;

	void RefreshBindings();
	void BindToGameState(AHBSoccerGameState* NewGameState);
	void UnbindFromGameState();
	void BindToPawn(APawn* NewPawn);
	void UnbindFromPawn();
	void RefreshVitalsFromAttributes();
	void RefreshAbilityCooldowns();
	void RefreshMatchValues();
	FString BuildTimerText() const;

	UFUNCTION()
	void HandleScoreChanged();

	UFUNCTION()
	void HandleMatchTimeChanged(int32 NewTimeSeconds);

	UFUNCTION()
	void HandleHeroAttributeChanged(EHBHeroAttribute Attribute, float OldValue, float NewValue);

	UFUNCTION()
	void HandleAbilityActivated(EHeroAbilitySlot AbilitySlot, APawn* ActivatingPawn);
};
