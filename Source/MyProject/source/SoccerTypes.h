#pragma once

#include "CoreMinimal.h"
#include "SoccerTypes.generated.h"

UENUM(BlueprintType)
enum class ETeam : uint8
{
	None UMETA(DisplayName = "None"),
	TeamA UMETA(DisplayName = "Team A"),
	TeamB UMETA(DisplayName = "Team B")
};

UENUM(BlueprintType)
enum class EMatchPhase : uint8
{
	None UMETA(DisplayName = "None"),
	Warmup UMETA(DisplayName = "Warmup"),
	Countdown UMETA(DisplayName = "Countdown"),
	ActiveMatch UMETA(DisplayName = "Active Match"),
	Overtime UMETA(DisplayName = "Overtime"),
	Ended UMETA(DisplayName = "Ended")
};
