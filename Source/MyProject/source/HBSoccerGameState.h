#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SoccerTypes.h"
#include "HBSoccerGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnScoreChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchTimeChanged, int32, NewTimeSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGoalCelebration, ETeam, ScoringTeam, int32, TeamAScore, int32, TeamBScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchPhaseChanged, EMatchPhase, NewMatchPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEndMatchScreenVisibilityChanged, bool, bIsVisible);

UCLASS()
class MYPROJECT_API AHBSoccerGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AHBSoccerGameState();

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_TeamAScore, Category = "Match")
	int32 TeamAScore = 0;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_TeamBScore, Category = "Match")
	int32 TeamBScore = 0;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_RemainingMatchTime, Category = "Match")
	int32 RemainingMatchTime = 0;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MatchPhase, Category = "Match")
	EMatchPhase MatchPhase = EMatchPhase::None;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ShowEndMatchScreen, Category = "Match")
	bool bShowEndMatchScreen = false;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnScoreChanged OnScoreChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMatchTimeChanged OnMatchTimeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnGoalCelebration OnGoalCelebration;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMatchPhaseChanged OnMatchPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEndMatchScreenVisibilityChanged OnEndMatchScreenVisibilityChanged;

	UFUNCTION(BlueprintPure, Category = "Match")
	int32 GetScoreForTeam(ETeam Team) const;

	void AddScore(ETeam Team, int32 Delta);
	void SetRemainingMatchTime(int32 NewTime);
	void SetMatchPhase(EMatchPhase NewMatchPhase);
	void SetShowEndMatchScreen(bool bNewShowEndMatchScreen);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastGoalCelebration(ETeam ScoringTeam, int32 NewTeamAScore, int32 NewTeamBScore);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_TeamAScore();

	UFUNCTION()
	void OnRep_TeamBScore();

	UFUNCTION()
	void OnRep_RemainingMatchTime();

	UFUNCTION()
	void OnRep_MatchPhase();

	UFUNCTION()
	void OnRep_ShowEndMatchScreen();
};
