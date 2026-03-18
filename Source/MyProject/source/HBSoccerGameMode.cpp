#include "source/HBSoccerGameMode.h"

#include "EngineUtils.h"
#include "source/HBCharacterBase.h"
#include "source/HBSoccerBall.h"
#include "source/HBSoccerGameState.h"
#include "source/HBSoccerHUD.h"
#include "source/HBSoccerPlayerController.h"
#include "source/HBSoccerPlayerState.h"
#include "source/HBTeamPlayerStart.h"
#include "TimerManager.h"

AHBSoccerGameMode::AHBSoccerGameMode()
{
	GameStateClass = AHBSoccerGameState::StaticClass();
	PlayerStateClass = AHBSoccerPlayerState::StaticClass();
	PlayerControllerClass = AHBSoccerPlayerController::StaticClass();
	HUDClass = AHBSoccerHUD::StaticClass();
	DefaultPawnClass = AHBCharacterBase::StaticClass();
}

void AHBSoccerGameMode::StartPlay()
{
	Super::StartPlay();

	StartWarmupPhase();
}

void AHBSoccerGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AHBSoccerPlayerState* SoccerPlayerState = NewPlayer ? NewPlayer->GetPlayerState<AHBSoccerPlayerState>() : nullptr;
	if (SoccerPlayerState)
	{
		AssignTeam(SoccerPlayerState);
	}

	RestartPlayer(NewPlayer);
	ApplyTeamToPawn(NewPlayer);
}

void AHBSoccerGameMode::Logout(AController* Exiting)
{
	RequestedHeroes.Remove(Cast<APlayerController>(Exiting));
	Super::Logout(Exiting);
}

UClass* AHBSoccerGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (APlayerController* PlayerController = Cast<APlayerController>(InController))
	{
		if (const AHBSoccerPlayerState* SoccerPlayerState = PlayerController->GetPlayerState<AHBSoccerPlayerState>())
		{
			if (SoccerPlayerState->SelectedHeroClass && IsHeroClassAllowed(SoccerPlayerState->SelectedHeroClass))
			{
				return SoccerPlayerState->SelectedHeroClass;
			}
		}

		if (const TSubclassOf<APawn>* RequestedClass = RequestedHeroes.Find(PlayerController))
		{
			if (*RequestedClass)
			{
				return *RequestedClass;
			}
		}
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

AActor* AHBSoccerGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	ETeam PlayerTeam = ETeam::None;
	if (const AHBSoccerPlayerState* SoccerPlayerState = Player ? Player->GetPlayerState<AHBSoccerPlayerState>() : nullptr)
	{
		PlayerTeam = SoccerPlayerState->Team;
	}

	TArray<AHBTeamPlayerStart*> TeamStarts;
	TArray<AHBTeamPlayerStart*> NeutralStarts;
	for (TActorIterator<AHBTeamPlayerStart> It(GetWorld()); It; ++It)
	{
		AHBTeamPlayerStart* TeamStart = *It;
		if (!TeamStart)
		{
			continue;
		}

		if (TeamStart->Team == PlayerTeam)
		{
			TeamStarts.Add(TeamStart);
		}
		else if (TeamStart->Team == ETeam::None)
		{
			NeutralStarts.Add(TeamStart);
		}
	}

	if (TeamStarts.Num() > 0)
	{
		return TeamStarts[FMath::RandRange(0, TeamStarts.Num() - 1)];
	}

	if (NeutralStarts.Num() > 0)
	{
		return NeutralStarts[FMath::RandRange(0, NeutralStarts.Num() - 1)];
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

void AHBSoccerGameMode::AssignTeam(AHBSoccerPlayerState* PlayerState) const
{
	if (!PlayerState)
	{
		return;
	}

	TryAssignTeam(PlayerState, ETeam::None);
}

void AHBSoccerGameMode::ApplyTeamToPawn(AController* Controller) const
{
	if (!Controller)
	{
		return;
	}

	AHBCharacterBase* HeroPawn = Cast<AHBCharacterBase>(Controller->GetPawn());
	const AHBSoccerPlayerState* SoccerState = Controller->GetPlayerState<AHBSoccerPlayerState>();
	if (HeroPawn && SoccerState)
	{
		HeroPawn->SetTeam(SoccerState->Team);
	}
}

bool AHBSoccerGameMode::HandleGoalScored(AHBSoccerBall* ScoringBall, ETeam GoalTeam)
{
	if (!HasAuthority() || HasMatchEnded())
	{
		return false;
	}

	AHBSoccerGameState* SoccerGameState = GetGameState<AHBSoccerGameState>();
	if (!IsGoalScoringPhase(SoccerGameState))
	{
		return false;
	}

	ETeam ScoringTeam = ETeam::None;
	if (!ResolveScoringTeam(ScoringBall, GoalTeam, ScoringTeam))
	{
		return false;
	}

	SoccerGameState->AddScore(ScoringTeam, 1);
	ScoringBall->ServerResetBallToCenter();
	ResetAllPlayerPositions();
	SoccerGameState->MulticastGoalCelebration(ScoringTeam, SoccerGameState->TeamAScore, SoccerGameState->TeamBScore);
	return true;
}

bool AHBSoccerGameMode::ResolveScoringTeam(AHBSoccerBall* ScoringBall, ETeam GoalTeam, ETeam& OutScoringTeam) const
{
	OutScoringTeam = ETeam::None;
	if (!ScoringBall)
	{
		return false;
	}

	if (GoalTeam == ETeam::TeamA)
	{
		OutScoringTeam = ETeam::TeamB;
	}
	else if (GoalTeam == ETeam::TeamB)
	{
		OutScoringTeam = ETeam::TeamA;
	}
	else
	{
		OutScoringTeam = ScoringBall->LastTouchTeam;
	}

	return OutScoringTeam != ETeam::None;
}

bool AHBSoccerGameMode::IsHeroClassAllowed(TSubclassOf<APawn> HeroClass) const
{
	if (!*HeroClass)
	{
		return false;
	}

	if (AllowedHeroClasses.Num() == 0)
	{
		return true;
	}

	for (const TSubclassOf<APawn>& AllowedClass : AllowedHeroClasses)
	{
		if (!*AllowedClass)
		{
			continue;
		}

		if (HeroClass == AllowedClass)
		{
			return true;
		}
	}

	return false;
}

bool AHBSoccerGameMode::CanAcceptTeamSelectionRequests() const
{
	const AHBSoccerGameState* SoccerGameState = GetGameState<AHBSoccerGameState>();
	if (!SoccerGameState)
	{
		return true;
	}

	return SoccerGameState->MatchPhase == EMatchPhase::None ||
		SoccerGameState->MatchPhase == EMatchPhase::Warmup ||
		SoccerGameState->MatchPhase == EMatchPhase::Countdown;
}

int32 AHBSoccerGameMode::GetTeamCount(ETeam Team, const AHBSoccerPlayerState* IgnorePlayerState) const
{
	if (!GameState || Team == ETeam::None)
	{
		return 0;
	}

	int32 Count = 0;
	for (APlayerState* ExistingState : GameState->PlayerArray)
	{
		const AHBSoccerPlayerState* SoccerState = Cast<AHBSoccerPlayerState>(ExistingState);
		if (!SoccerState || SoccerState == IgnorePlayerState)
		{
			continue;
		}

		if (SoccerState->Team == Team)
		{
			++Count;
		}
	}

	return Count;
}

bool AHBSoccerGameMode::CanJoinTeam(ETeam Team, const AHBSoccerPlayerState* IgnorePlayerState) const
{
	if (Team != ETeam::TeamA && Team != ETeam::TeamB)
	{
		return false;
	}

	return GetTeamCount(Team, IgnorePlayerState) < MaxPlayersPerTeam;
}

bool AHBSoccerGameMode::TryAssignTeam(AHBSoccerPlayerState* PlayerState, ETeam RequestedTeam) const
{
	if (!PlayerState)
	{
		return false;
	}

	const ETeam CurrentTeam = PlayerState->Team;
	if ((RequestedTeam == ETeam::TeamA || RequestedTeam == ETeam::TeamB) &&
		(CurrentTeam == RequestedTeam || CanJoinTeam(RequestedTeam, PlayerState)))
	{
		PlayerState->SetTeam(RequestedTeam);
		return true;
	}

	const int32 TeamACount = GetTeamCount(ETeam::TeamA, PlayerState);
	const int32 TeamBCount = GetTeamCount(ETeam::TeamB, PlayerState);
	const ETeam PreferredTeam = (TeamACount <= TeamBCount) ? ETeam::TeamA : ETeam::TeamB;
	const ETeam AlternateTeam = (PreferredTeam == ETeam::TeamA) ? ETeam::TeamB : ETeam::TeamA;

	if (CanJoinTeam(PreferredTeam, PlayerState))
	{
		PlayerState->SetTeam(PreferredTeam);
		return true;
	}

	if (CanJoinTeam(AlternateTeam, PlayerState))
	{
		PlayerState->SetTeam(AlternateTeam);
		return true;
	}

	return false;
}

void AHBSoccerGameMode::ResetAllPlayerPositions()
{
	if (!GetWorld())
	{
		return;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!PlayerController)
		{
			continue;
		}

		RestartPlayer(PlayerController);
		ApplyTeamToPawn(PlayerController);
	}
}

void AHBSoccerGameMode::ResetAllSoccerBallsToCenter() const
{
	if (!GetWorld())
	{
		return;
	}

	for (TActorIterator<AHBSoccerBall> It(GetWorld()); It; ++It)
	{
		AHBSoccerBall* SoccerBall = *It;
		if (SoccerBall)
		{
			SoccerBall->ServerResetBallToCenter();
		}
	}
}

bool AHBSoccerGameMode::IsGoalScoringPhase(const AHBSoccerGameState* SoccerGameState) const
{
	if (!SoccerGameState || SoccerGameState->RemainingMatchTime <= 0)
	{
		return false;
	}

	return SoccerGameState->MatchPhase == EMatchPhase::ActiveMatch || SoccerGameState->MatchPhase == EMatchPhase::Overtime;
}

void AHBSoccerGameMode::SetMatchPhaseWithDuration(EMatchPhase NewPhase, int32 DurationSeconds)
{
	AHBSoccerGameState* SoccerGameState = GetGameState<AHBSoccerGameState>();
	if (!SoccerGameState)
	{
		return;
	}

	SoccerGameState->SetMatchPhase(NewPhase);
	SoccerGameState->SetRemainingMatchTime(DurationSeconds);

	FTimerManagerTimerParameters TimerParams;
	TimerParams.bLoop = true;
	GetWorldTimerManager().SetTimer(MatchTimerHandle, this, &AHBSoccerGameMode::TickMatchPhase, 1.0f, TimerParams);
}

void AHBSoccerGameMode::StartWarmupPhase()
{
	AHBSoccerGameState* SoccerGameState = GetGameState<AHBSoccerGameState>();
	if (SoccerGameState)
	{
		SoccerGameState->SetShowEndMatchScreen(false);
		SoccerGameState->TeamAScore = 0;
		SoccerGameState->TeamBScore = 0;
		SoccerGameState->OnScoreChanged.Broadcast();
	}

	ResetAllSoccerBallsToCenter();
	ResetAllPlayerPositions();
	SetMatchPhaseWithDuration(EMatchPhase::Warmup, FMath::Max(0, WarmupLengthSeconds));
}

void AHBSoccerGameMode::StartCountdownPhase()
{
	ResetAllSoccerBallsToCenter();
	ResetAllPlayerPositions();
	SetMatchPhaseWithDuration(EMatchPhase::Countdown, FMath::Max(0, CountdownLengthSeconds));
}

void AHBSoccerGameMode::StartActiveMatchPhase()
{
	ResetAllSoccerBallsToCenter();
	ResetAllPlayerPositions();
	SetMatchPhaseWithDuration(EMatchPhase::ActiveMatch, FMath::Max(0, MatchLengthSeconds));
}

void AHBSoccerGameMode::StartOvertimePhase()
{
	ResetAllSoccerBallsToCenter();
	ResetAllPlayerPositions();
	SetMatchPhaseWithDuration(EMatchPhase::Overtime, FMath::Max(0, OvertimeLengthSeconds));
}

void AHBSoccerGameMode::StartEndedPhase()
{
	GetWorldTimerManager().ClearTimer(MatchTimerHandle);

	if (AHBSoccerGameState* SoccerGameState = GetGameState<AHBSoccerGameState>())
	{
		SoccerGameState->SetMatchPhase(EMatchPhase::Ended);
		SoccerGameState->SetRemainingMatchTime(0);
		SoccerGameState->SetShowEndMatchScreen(true);
	}

	if (!HasMatchEnded())
	{
		EndMatch();
	}
}

void AHBSoccerGameMode::TickMatchPhase()
{
	AHBSoccerGameState* SoccerGameState = GetGameState<AHBSoccerGameState>();
	if (!SoccerGameState)
	{
		return;
	}

	const int32 NewTime = SoccerGameState->RemainingMatchTime - 1;
	SoccerGameState->SetRemainingMatchTime(NewTime);
	if (SoccerGameState->RemainingMatchTime > 0)
	{
		return;
	}

	switch (SoccerGameState->MatchPhase)
	{
	case EMatchPhase::Warmup:
		StartCountdownPhase();
		break;
	case EMatchPhase::Countdown:
		StartActiveMatchPhase();
		break;
	case EMatchPhase::ActiveMatch:
		if (SoccerGameState->TeamAScore == SoccerGameState->TeamBScore && OvertimeLengthSeconds > 0)
		{
			StartOvertimePhase();
		}
		else
		{
			StartEndedPhase();
		}
		break;
	case EMatchPhase::Overtime:
		StartEndedPhase();
		break;
	default:
		StartEndedPhase();
		break;
	}
}

void AHBSoccerGameMode::HandleTeamSelectionRequest(AHBSoccerPlayerController* PlayerController, ETeam RequestedTeam, TSubclassOf<APawn> HeroClass)
{
	if (!HasAuthority() || !PlayerController)
	{
		return;
	}

	if (!CanAcceptTeamSelectionRequests())
	{
		return;
	}

	AHBSoccerPlayerState* SoccerPlayerState = PlayerController->GetPlayerState<AHBSoccerPlayerState>();
	if (!SoccerPlayerState)
	{
		return;
	}

	const ETeam PreviousTeam = SoccerPlayerState->Team;
	const TSubclassOf<APawn> PreviousHeroClass = SoccerPlayerState->SelectedHeroClass;

	TryAssignTeam(SoccerPlayerState, RequestedTeam);

	bool bHeroChanged = false;
	const UClass* RequestedHeroClass = *HeroClass;
	const bool bHeroClassFlagsValid = RequestedHeroClass &&
		!RequestedHeroClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists);

	if (RequestedHeroClass && bHeroClassFlagsValid && IsHeroClassAllowed(HeroClass))
	{
		SoccerPlayerState->SetSelectedHeroClass(HeroClass);
		RequestedHeroes.FindOrAdd(PlayerController) = HeroClass;
		bHeroChanged = SoccerPlayerState->SelectedHeroClass != PreviousHeroClass;
	}

	const bool bTeamChanged = SoccerPlayerState->Team != PreviousTeam;
	if (!bTeamChanged && !bHeroChanged)
	{
		return;
	}

	RestartPlayer(PlayerController);
	ApplyTeamToPawn(PlayerController);
}

void AHBSoccerGameMode::SetRequestedHeroClass(AHBSoccerPlayerController* PlayerController, TSubclassOf<APawn> HeroClass)
{
	if (!PlayerController || !*HeroClass)
	{
		return;
	}

	ETeam RequestedTeam = ETeam::None;
	if (const AHBSoccerPlayerState* SoccerPlayerState = PlayerController->GetPlayerState<AHBSoccerPlayerState>())
	{
		RequestedTeam = SoccerPlayerState->Team;
	}

	HandleTeamSelectionRequest(PlayerController, RequestedTeam, HeroClass);
}
