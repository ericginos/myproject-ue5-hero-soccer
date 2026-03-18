#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "SoccerTypes.h"
#include "HBTeamPlayerStart.generated.h"

UCLASS()
class MYPROJECT_API AHBTeamPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Team Spawn")
	ETeam Team = ETeam::None;
};
