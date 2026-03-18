#include "source/HBSoccerGameState.h"

#include "Net/UnrealNetwork.h"

AHBSoccerGameState::AHBSoccerGameState()
{
	bReplicates = true;
}

int32 AHBSoccerGameState::GetScoreForTeam(ETeam Team) const
{
	switch (Team)
	{
	case ETeam::TeamA:
		return TeamAScore;
	case ETeam::TeamB:
		return TeamBScore;
	default:
		return 0;
	}
}

void AHBSoccerGameState::AddScore(ETeam Team, int32 Delta)
{
	if (!HasAuthority())
	{
		return;
	}

	if (Team == ETeam::TeamA)
	{
		TeamAScore += Delta;
	}
	else if (Team == ETeam::TeamB)
	{
		TeamBScore += Delta;
	}

	OnScoreChanged.Broadcast();
}

void AHBSoccerGameState::SetRemainingMatchTime(int32 NewTime)
{
	if (!HasAuthority())
	{
		return;
	}

	RemainingMatchTime = FMath::Max(0, NewTime);
	OnMatchTimeChanged.Broadcast(RemainingMatchTime);
}

void AHBSoccerGameState::SetMatchPhase(EMatchPhase NewMatchPhase)
{
	if (!HasAuthority())
	{
		return;
	}

	MatchPhase = NewMatchPhase;
	OnMatchPhaseChanged.Broadcast(MatchPhase);
}

void AHBSoccerGameState::SetShowEndMatchScreen(bool bNewShowEndMatchScreen)
{
	if (!HasAuthority())
	{
		return;
	}

	bShowEndMatchScreen = bNewShowEndMatchScreen;
	OnEndMatchScreenVisibilityChanged.Broadcast(bShowEndMatchScreen);
}

void AHBSoccerGameState::MulticastGoalCelebration_Implementation(ETeam ScoringTeam, int32 NewTeamAScore, int32 NewTeamBScore)
{
	OnGoalCelebration.Broadcast(ScoringTeam, NewTeamAScore, NewTeamBScore);
}

void AHBSoccerGameState::OnRep_TeamAScore()
{
	OnScoreChanged.Broadcast();
}

void AHBSoccerGameState::OnRep_TeamBScore()
{
	OnScoreChanged.Broadcast();
}

void AHBSoccerGameState::OnRep_RemainingMatchTime()
{
	OnMatchTimeChanged.Broadcast(RemainingMatchTime);
}

void AHBSoccerGameState::OnRep_MatchPhase()
{
	OnMatchPhaseChanged.Broadcast(MatchPhase);
}

void AHBSoccerGameState::OnRep_ShowEndMatchScreen()
{
	OnEndMatchScreenVisibilityChanged.Broadcast(bShowEndMatchScreen);
}

void AHBSoccerGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHBSoccerGameState, TeamAScore);
	DOREPLIFETIME(AHBSoccerGameState, TeamBScore);
	DOREPLIFETIME(AHBSoccerGameState, RemainingMatchTime);
	DOREPLIFETIME(AHBSoccerGameState, MatchPhase);
	DOREPLIFETIME(AHBSoccerGameState, bShowEndMatchScreen);
}
