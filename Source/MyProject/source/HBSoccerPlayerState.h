#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SoccerTypes.h"
#include "HBSoccerPlayerState.generated.h"

class APawn;

UCLASS()
class MYPROJECT_API AHBSoccerPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Team, Category = "Team")
	ETeam Team = ETeam::None;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Stats")
	int32 Goals = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Stats")
	int32 Assists = 0;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SelectedHeroClass, Category = "Lobby")
	TSubclassOf<APawn> SelectedHeroClass;

	void SetTeam(ETeam NewTeam);
	void SetSelectedHeroClass(TSubclassOf<APawn> NewHeroClass);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_Team();

	UFUNCTION()
	void OnRep_SelectedHeroClass();
};
