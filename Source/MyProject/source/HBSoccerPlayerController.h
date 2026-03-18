#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SoccerTypes.h"
#include "HBSoccerPlayerController.generated.h"

class APawn;
class UHBHeroSelectionWidget;

UCLASS()
class MYPROJECT_API AHBSoccerPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AHBSoccerPlayerController();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UHBHeroSelectionWidget> HeroSelectionWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UHBHeroSelectionWidget> HeroSelectionWidget = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void SelectTeamAndHero(ETeam RequestedTeam, TSubclassOf<APawn> HeroClass);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowHeroSelectionScreen();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideHeroSelectionScreen();

	UFUNCTION(Server, Reliable)
	void Server_RequestHero(TSubclassOf<APawn> HeroClass);

	UFUNCTION(Server, Reliable)
	void Server_SelectTeamAndHero(ETeam RequestedTeam, TSubclassOf<APawn> HeroClass);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleMatchPhaseChanged(EMatchPhase NewMatchPhase);
};
