#include "source/HBSoccerHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "source/HBSoccerGameState.h"

AHBSoccerHUD::AHBSoccerHUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHBSoccerHUD::BeginPlay()
{
	Super::BeginPlay();
	RefreshBindings();
	RefreshMatchValues();
	RefreshVitalsFromAttributes();
	RefreshAbilityCooldowns();
}

void AHBSoccerHUD::DrawHUD()
{
	Super::DrawHUD();
	RefreshBindings();
	RefreshAbilityCooldowns();

	if (!Canvas || !GEngine)
	{
		return;
	}

	const UFont* DrawFont = GEngine->GetSmallFont();
	if (!DrawFont)
	{
		return;
	}

	const float BaseX = 42.0f;
	float BaseY = 38.0f;
	const float LineSpacing = 20.0f;

	Canvas->DrawText(DrawFont, FString::Printf(TEXT("Health: %.0f"), Health), BaseX, BaseY);
	BaseY += LineSpacing;
	Canvas->DrawText(DrawFont, FString::Printf(TEXT("Stamina: %.0f"), Stamina), BaseX, BaseY);
	BaseY += (LineSpacing + 6.0f);

	Canvas->DrawText(DrawFont, FString::Printf(TEXT("Ability 1 CD: %.1f"), AbilityOneCooldownRemaining), BaseX, BaseY);
	BaseY += LineSpacing;
	Canvas->DrawText(DrawFont, FString::Printf(TEXT("Ability 2 CD: %.1f"), AbilityTwoCooldownRemaining), BaseX, BaseY);
	BaseY += LineSpacing;
	Canvas->DrawText(DrawFont, FString::Printf(TEXT("Ability 3 CD: %.1f"), AbilityThreeCooldownRemaining), BaseX, BaseY);

	const float TopCenterX = FMath::Max(24.0f, (Canvas->ClipX * 0.5f) - 140.0f);
	const float TopY = 24.0f;
	Canvas->DrawText(
		DrawFont,
		FString::Printf(TEXT("Team A %d  -  %d Team B"), TeamAScore, TeamBScore),
		TopCenterX,
		TopY);
	Canvas->DrawText(DrawFont, BuildTimerText(), TopCenterX + 64.0f, TopY + LineSpacing);
}

void AHBSoccerHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromPawn();
	UnbindFromGameState();
	Super::EndPlay(EndPlayReason);
}

void AHBSoccerHUD::RefreshBindings()
{
	if (!PlayerOwner)
	{
		return;
	}

	AHBSoccerGameState* CurrentGameState = PlayerOwner->GetWorld() ? PlayerOwner->GetWorld()->GetGameState<AHBSoccerGameState>() : nullptr;
	if (CurrentGameState != CachedGameState.Get())
	{
		BindToGameState(CurrentGameState);
	}

	APawn* CurrentPawn = PlayerOwner->GetPawn();
	if (CurrentPawn != CachedPawn.Get())
	{
		BindToPawn(CurrentPawn);
	}
}

void AHBSoccerHUD::BindToGameState(AHBSoccerGameState* NewGameState)
{
	UnbindFromGameState();
	CachedGameState = NewGameState;
	if (!NewGameState)
	{
		TeamAScore = 0;
		TeamBScore = 0;
		MatchTimeSeconds = 0;
		return;
	}

	NewGameState->OnScoreChanged.RemoveDynamic(this, &AHBSoccerHUD::HandleScoreChanged);
	NewGameState->OnMatchTimeChanged.RemoveDynamic(this, &AHBSoccerHUD::HandleMatchTimeChanged);
	NewGameState->OnScoreChanged.AddDynamic(this, &AHBSoccerHUD::HandleScoreChanged);
	NewGameState->OnMatchTimeChanged.AddDynamic(this, &AHBSoccerHUD::HandleMatchTimeChanged);
	RefreshMatchValues();
}

void AHBSoccerHUD::UnbindFromGameState()
{
	AHBSoccerGameState* GameState = CachedGameState.Get();
	if (!GameState)
	{
		CachedGameState = nullptr;
		return;
	}

	GameState->OnScoreChanged.RemoveDynamic(this, &AHBSoccerHUD::HandleScoreChanged);
	GameState->OnMatchTimeChanged.RemoveDynamic(this, &AHBSoccerHUD::HandleMatchTimeChanged);
	CachedGameState = nullptr;
}

void AHBSoccerHUD::BindToPawn(APawn* NewPawn)
{
	UnbindFromPawn();
	CachedPawn = NewPawn;
	if (!NewPawn)
	{
		Health = 0.0f;
		Stamina = 0.0f;
		AbilityOneCooldownRemaining = 0.0f;
		AbilityTwoCooldownRemaining = 0.0f;
		AbilityThreeCooldownRemaining = 0.0f;
		return;
	}

	CachedAttributeComponent = NewPawn->FindComponentByClass<UHBAttributeComponent>();
	CachedAbilityComponent = NewPawn->FindComponentByClass<UHBAbilityComponent>();

	if (UHBAttributeComponent* AttributeComponent = CachedAttributeComponent.Get())
	{
		AttributeComponent->OnHeroAttributeChanged.RemoveDynamic(this, &AHBSoccerHUD::HandleHeroAttributeChanged);
		AttributeComponent->OnHeroAttributeChanged.AddDynamic(this, &AHBSoccerHUD::HandleHeroAttributeChanged);
	}

	if (UHBAbilityComponent* AbilityComponent = CachedAbilityComponent.Get())
	{
		AbilityComponent->OnHeroAbilityActivated.RemoveDynamic(this, &AHBSoccerHUD::HandleAbilityActivated);
		AbilityComponent->OnHeroAbilityActivated.AddDynamic(this, &AHBSoccerHUD::HandleAbilityActivated);
	}

	RefreshVitalsFromAttributes();
	RefreshAbilityCooldowns();
}

void AHBSoccerHUD::UnbindFromPawn()
{
	if (UHBAttributeComponent* AttributeComponent = CachedAttributeComponent.Get())
	{
		AttributeComponent->OnHeroAttributeChanged.RemoveDynamic(this, &AHBSoccerHUD::HandleHeroAttributeChanged);
	}

	if (UHBAbilityComponent* AbilityComponent = CachedAbilityComponent.Get())
	{
		AbilityComponent->OnHeroAbilityActivated.RemoveDynamic(this, &AHBSoccerHUD::HandleAbilityActivated);
	}

	CachedPawn = nullptr;
	CachedAttributeComponent = nullptr;
	CachedAbilityComponent = nullptr;
}

void AHBSoccerHUD::RefreshVitalsFromAttributes()
{
	const UHBAttributeComponent* AttributeComponent = CachedAttributeComponent.Get();
	if (!AttributeComponent)
	{
		Health = 0.0f;
		Stamina = 0.0f;
		return;
	}

	Health = AttributeComponent->GetAttributeValue(EHBHeroAttribute::Health);
	Stamina = AttributeComponent->GetAttributeValue(EHBHeroAttribute::Stamina);
}

void AHBSoccerHUD::RefreshAbilityCooldowns()
{
	const UHBAbilityComponent* AbilityComponent = CachedAbilityComponent.Get();
	if (!AbilityComponent)
	{
		AbilityOneCooldownRemaining = 0.0f;
		AbilityTwoCooldownRemaining = 0.0f;
		AbilityThreeCooldownRemaining = 0.0f;
		return;
	}

	AbilityOneCooldownRemaining = AbilityComponent->GetRemainingCooldown(EHeroAbilitySlot::AbilityOne);
	AbilityTwoCooldownRemaining = AbilityComponent->GetRemainingCooldown(EHeroAbilitySlot::AbilityTwo);
	AbilityThreeCooldownRemaining = AbilityComponent->GetRemainingCooldown(EHeroAbilitySlot::AbilityThree);
}

void AHBSoccerHUD::RefreshMatchValues()
{
	const AHBSoccerGameState* GameState = CachedGameState.Get();
	if (!GameState)
	{
		TeamAScore = 0;
		TeamBScore = 0;
		MatchTimeSeconds = 0;
		return;
	}

	TeamAScore = GameState->TeamAScore;
	TeamBScore = GameState->TeamBScore;
	MatchTimeSeconds = GameState->RemainingMatchTime;
}

FString AHBSoccerHUD::BuildTimerText() const
{
	const int32 SafeTime = FMath::Max(0, MatchTimeSeconds);
	const int32 Minutes = SafeTime / 60;
	const int32 Seconds = SafeTime % 60;
	return FString::Printf(TEXT("Time: %02d:%02d"), Minutes, Seconds);
}

void AHBSoccerHUD::HandleScoreChanged()
{
	RefreshMatchValues();
}

void AHBSoccerHUD::HandleMatchTimeChanged(int32 NewTimeSeconds)
{
	MatchTimeSeconds = FMath::Max(0, NewTimeSeconds);
}

void AHBSoccerHUD::HandleHeroAttributeChanged(EHBHeroAttribute Attribute, float OldValue, float NewValue)
{
	if (Attribute == EHBHeroAttribute::Health)
	{
		Health = NewValue;
	}
	else if (Attribute == EHBHeroAttribute::Stamina)
	{
		Stamina = NewValue;
	}
}

void AHBSoccerHUD::HandleAbilityActivated(EHeroAbilitySlot AbilitySlot, APawn* ActivatingPawn)
{
	RefreshAbilityCooldowns();
}
