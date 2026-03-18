#include "source/HBSoccerPlayerState.h"

#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

void AHBSoccerPlayerState::SetTeam(ETeam NewTeam)
{
	if (!HasAuthority())
	{
		return;
	}

	Team = NewTeam;
	OnRep_Team();
}

void AHBSoccerPlayerState::SetSelectedHeroClass(TSubclassOf<APawn> NewHeroClass)
{
	if (!HasAuthority())
	{
		return;
	}

	SelectedHeroClass = NewHeroClass;
	OnRep_SelectedHeroClass();
}

void AHBSoccerPlayerState::OnRep_Team()
{
	// Bind team-specific UI/cosmetics here.
}

void AHBSoccerPlayerState::OnRep_SelectedHeroClass()
{
	// Hook for lobby UI hero portrait/state updates.
}

void AHBSoccerPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHBSoccerPlayerState, Team);
	DOREPLIFETIME(AHBSoccerPlayerState, Goals);
	DOREPLIFETIME(AHBSoccerPlayerState, Assists);
	DOREPLIFETIME(AHBSoccerPlayerState, SelectedHeroClass);
}
