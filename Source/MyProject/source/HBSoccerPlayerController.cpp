#include "source/HBSoccerPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "source/HBHeroSelectionWidget.h"
#include "source/HBSoccerGameMode.h"
#include "source/HBSoccerGameState.h"
#include "source/HBSoccerPlayerState.h"
#include "Engine/World.h"

namespace
{
ETeam SanitizeRequestedTeam(const ETeam RequestedTeam)
{
	switch (RequestedTeam)
	{
	case ETeam::TeamA:
	case ETeam::TeamB:
	case ETeam::None:
		return RequestedTeam;
	default:
		return ETeam::None;
	}
}

bool IsSafeHeroClassSelection(const TSubclassOf<APawn> HeroClass)
{
	if (!*HeroClass)
	{
		return true;
	}

	const UClass* HeroUClass = *HeroClass;
	return HeroUClass &&
		HeroUClass->IsChildOf(APawn::StaticClass()) &&
		!HeroUClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists);
}
}

AHBSoccerPlayerController::AHBSoccerPlayerController()
{
	HeroSelectionWidgetClass = UHBHeroSelectionWidget::StaticClass();
}

void AHBSoccerPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (AHBSoccerGameState* SoccerGameState = GetWorld() ? GetWorld()->GetGameState<AHBSoccerGameState>() : nullptr)
	{
		SoccerGameState->OnMatchPhaseChanged.RemoveDynamic(this, &AHBSoccerPlayerController::HandleMatchPhaseChanged);
		SoccerGameState->OnMatchPhaseChanged.AddDynamic(this, &AHBSoccerPlayerController::HandleMatchPhaseChanged);
		HandleMatchPhaseChanged(SoccerGameState->MatchPhase);
	}
	else if (IsLocalController())
	{
		ShowHeroSelectionScreen();
	}
}

void AHBSoccerPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AHBSoccerGameState* SoccerGameState = GetWorld() ? GetWorld()->GetGameState<AHBSoccerGameState>() : nullptr)
	{
		SoccerGameState->OnMatchPhaseChanged.RemoveDynamic(this, &AHBSoccerPlayerController::HandleMatchPhaseChanged);
	}

	HideHeroSelectionScreen();
	Super::EndPlay(EndPlayReason);
}

void AHBSoccerPlayerController::SelectTeamAndHero(ETeam RequestedTeam, TSubclassOf<APawn> HeroClass)
{
	const ETeam SanitizedTeam = SanitizeRequestedTeam(RequestedTeam);
	if (!IsSafeHeroClassSelection(HeroClass))
	{
		return;
	}

	if (!HasAuthority())
	{
		Server_SelectTeamAndHero(SanitizedTeam, HeroClass);
		return;
	}

	if (AHBSoccerGameMode* SoccerGameMode = Cast<AHBSoccerGameMode>(GetWorld()->GetAuthGameMode()))
	{
		SoccerGameMode->HandleTeamSelectionRequest(this, SanitizedTeam, HeroClass);
	}
}

void AHBSoccerPlayerController::ShowHeroSelectionScreen()
{
	if (!IsLocalController())
	{
		return;
	}

	if (!HeroSelectionWidget && HeroSelectionWidgetClass)
	{
		HeroSelectionWidget = CreateWidget<UHBHeroSelectionWidget>(this, HeroSelectionWidgetClass);
	}

	if (HeroSelectionWidget && !HeroSelectionWidget->IsInViewport())
	{
		HeroSelectionWidget->AddToViewport(50);
	}
}

void AHBSoccerPlayerController::HideHeroSelectionScreen()
{
	if (HeroSelectionWidget && HeroSelectionWidget->IsInViewport())
	{
		HeroSelectionWidget->RemoveFromParent();
	}
}

void AHBSoccerPlayerController::Server_RequestHero_Implementation(TSubclassOf<APawn> HeroClass)
{
	if (!IsSafeHeroClassSelection(HeroClass))
	{
		return;
	}

	ETeam RequestedTeam = ETeam::None;
	if (const AHBSoccerPlayerState* SoccerPlayerState = GetPlayerState<AHBSoccerPlayerState>())
	{
		RequestedTeam = SoccerPlayerState->Team;
	}

	SelectTeamAndHero(SanitizeRequestedTeam(RequestedTeam), HeroClass);
}

void AHBSoccerPlayerController::Server_SelectTeamAndHero_Implementation(ETeam RequestedTeam, TSubclassOf<APawn> HeroClass)
{
	if (!IsSafeHeroClassSelection(HeroClass))
	{
		return;
	}

	SelectTeamAndHero(SanitizeRequestedTeam(RequestedTeam), HeroClass);
}

void AHBSoccerPlayerController::HandleMatchPhaseChanged(EMatchPhase NewMatchPhase)
{
	if (!IsLocalController())
	{
		return;
	}

	if (NewMatchPhase == EMatchPhase::ActiveMatch || NewMatchPhase == EMatchPhase::Overtime || NewMatchPhase == EMatchPhase::Ended)
	{
		HideHeroSelectionScreen();
	}
	else
	{
		ShowHeroSelectionScreen();
	}
}
