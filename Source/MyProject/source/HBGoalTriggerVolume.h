#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoccerTypes.h"
#include "HBGoalTriggerVolume.generated.h"

class UBoxComponent;

UCLASS()
class MYPROJECT_API AHBGoalTriggerVolume : public AActor
{
	GENERATED_BODY()

public:
	AHBGoalTriggerVolume();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal")
	TObjectPtr<UBoxComponent> GoalTrigger = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal")
	ETeam GoalTeam = ETeam::TeamA;
};
