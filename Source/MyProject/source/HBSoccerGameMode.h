#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SoccerTypes.h"
#include "HBSoccerGameMode.generated.h"

class AHBSoccerBall;
class AHBSoccerPlayerController;
class AHBSoccerPlayerState;
class AHBSoccerGameState;
class APawn;

UCLASS()
class MYPROJECT_API AHBSoccerGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AHBSoccerGameMode();

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 MaxPlayersPerTeam = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 MatchLengthSeconds = 600;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 WarmupLengthSeconds = 15;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 CountdownLengthSeconds = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 OvertimeLengthSeconds = 120;

	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	TArray<TSubclassOf<APawn>> AllowedHeroClasses;

	UFUNCTION(BlueprintCallable, Category = "Match")
	bool HandleGoalScored(AHBSoccerBall* ScoringBall, ETeam GoalTeam);

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void HandleTeamSelectionRequest(AHBSoccerPlayerController* PlayerController, ETeam RequestedTeam, TSubclassOf<APawn> HeroClass);

	void SetRequestedHeroClass(AHBSoccerPlayerController* PlayerController, TSubclassOf<APawn> HeroClass);

	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:
	FTimerHandle MatchTimerHandle;

	UPROPERTY(Transient)
	TMap<TWeakObjectPtr<APlayerController>, TSubclassOf<APawn>> RequestedHeroes;

	bool ResolveScoringTeam(AHBSoccerBall* ScoringBall, ETeam GoalTeam, ETeam& OutScoringTeam) const;
	bool IsHeroClassAllowed(TSubclassOf<APawn> HeroClass) const;
	bool CanAcceptTeamSelectionRequests() const;
	int32 GetTeamCount(ETeam Team, const AHBSoccerPlayerState* IgnorePlayerState = nullptr) const;
	bool CanJoinTeam(ETeam Team, const AHBSoccerPlayerState* IgnorePlayerState = nullptr) const;
	bool TryAssignTeam(AHBSoccerPlayerState* PlayerState, ETeam RequestedTeam) const;
	void ResetAllPlayerPositions();
	void ResetAllSoccerBallsToCenter() const;
	void AssignTeam(AHBSoccerPlayerState* PlayerState) const;
	void ApplyTeamToPawn(AController* Controller) const;
	bool IsGoalScoringPhase(const AHBSoccerGameState* SoccerGameState) const;
	void SetMatchPhaseWithDuration(EMatchPhase NewPhase, int32 DurationSeconds);
	void StartWarmupPhase();
	void StartCountdownPhase();
	void StartActiveMatchPhase();
	void StartOvertimePhase();
	void StartEndedPhase();
	void TickMatchPhase();
};
