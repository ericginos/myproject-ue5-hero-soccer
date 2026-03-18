#include "source/HBHeroSelectionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "source/DefenderHero.h"
#include "source/ForwardHero.h"
#include "source/GoalieHero.h"
#include "source/HBSoccerGameState.h"
#include "source/HBSoccerPlayerController.h"
#include "source/MidfielderHero.h"

void UHBHeroSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (HeroEntries.Num() == 0)
	{
		BuildDefaultHeroEntries();
	}

	BuildRuntimeWidgetTree();
	RefreshHeroChoiceOptions();
	RefreshRuntimeText();

	if (ConfirmSelectionButton)
	{
		ConfirmSelectionButton->OnClicked.RemoveDynamic(this, &UHBHeroSelectionWidget::HandleConfirmSelectionClicked);
		ConfirmSelectionButton->OnClicked.AddDynamic(this, &UHBHeroSelectionWidget::HandleConfirmSelectionClicked);
	}
}

bool UHBHeroSelectionWidget::SelectHero(TSubclassOf<APawn> HeroClass)
{
	return SelectHeroAndTeam(HeroClass, RequestedTeam);
}

bool UHBHeroSelectionWidget::SelectHeroAndTeam(TSubclassOf<APawn> HeroClass, ETeam TeamToJoin)
{
	if (!*HeroClass || !CanSubmitSelection())
	{
		return false;
	}

	const FHBHeroSelectionEntry* Entry = FindHeroEntry(HeroClass);
	if (!Entry || Entry->bLocked)
	{
		return false;
	}

	AHBSoccerPlayerController* SoccerPC = Cast<AHBSoccerPlayerController>(GetOwningPlayer());
	if (!SoccerPC)
	{
		return false;
	}

	RequestedTeam = TeamToJoin;
	SelectedHeroClass = HeroClass;
	SoccerPC->SelectTeamAndHero(RequestedTeam, SelectedHeroClass);
	RefreshRuntimeText();
	return true;
}

bool UHBHeroSelectionWidget::IsHeroLocked(TSubclassOf<APawn> HeroClass) const
{
	const FHBHeroSelectionEntry* Entry = FindHeroEntry(HeroClass);
	return Entry ? Entry->bLocked : true;
}

void UHBHeroSelectionWidget::BuildDefaultHeroEntries()
{
	HeroEntries.Reset();

	FHBHeroSelectionEntry GoalieEntry;
	GoalieEntry.HeroClass = AGoalieHero::StaticClass();
	GoalieEntry.HeroName = FText::FromString(TEXT("Goalie"));
	GoalieEntry.RoleCategory = EHBHeroRoleCategory::Goalie;
	GoalieEntry.AbilityOneDescription = FText::FromString(TEXT("Dive Save: rapid directional launch to deny shots."));
	GoalieEntry.AbilityTwoDescription = FText::FromString(TEXT("Goal Shield: temporary barrier in front of goal."));
	GoalieEntry.AbilityThreeDescription = FText::FromString(TEXT("Power Punt: long-range ball clear."));
	HeroEntries.Add(GoalieEntry);

	FHBHeroSelectionEntry DefenderEntry;
	DefenderEntry.HeroClass = ADefenderHero::StaticClass();
	DefenderEntry.HeroName = FText::FromString(TEXT("Defender"));
	DefenderEntry.RoleCategory = EHBHeroRoleCategory::Defender;
	DefenderEntry.AbilityOneDescription = FText::FromString(TEXT("Body Check: close-range knockback on enemies."));
	DefenderEntry.AbilityTwoDescription = FText::FromString(TEXT("Intercept Dash: forward burst to break plays."));
	DefenderEntry.AbilityThreeDescription = FText::FromString(TEXT("Defensive Aura: reduces allied incoming damage."));
	HeroEntries.Add(DefenderEntry);

	FHBHeroSelectionEntry MidfielderEntry;
	MidfielderEntry.HeroClass = AMidfielderHero::StaticClass();
	MidfielderEntry.HeroName = FText::FromString(TEXT("Midfielder"));
	MidfielderEntry.RoleCategory = EHBHeroRoleCategory::Midfielder;
	MidfielderEntry.AbilityOneDescription = FText::FromString(TEXT("Speed Boost Aura: team movement buff in radius."));
	MidfielderEntry.AbilityTwoDescription = FText::FromString(TEXT("Precision Pass: assisted pass to ally target."));
	MidfielderEntry.AbilityThreeDescription = FText::FromString(TEXT("Stamina Regen Field: restores nearby ally stamina."));
	HeroEntries.Add(MidfielderEntry);

	FHBHeroSelectionEntry ForwardEntry;
	ForwardEntry.HeroClass = AForwardHero::StaticClass();
	ForwardEntry.HeroName = FText::FromString(TEXT("Forward"));
	ForwardEntry.RoleCategory = EHBHeroRoleCategory::Forward;
	ForwardEntry.AbilityOneDescription = FText::FromString(TEXT("Power Shot: heavy straight strike."));
	ForwardEntry.AbilityTwoDescription = FText::FromString(TEXT("Curve Shot: controlled curved strike."));
	ForwardEntry.AbilityThreeDescription = FText::FromString(TEXT("Breakaway Dash: burst for separation."));
	HeroEntries.Add(ForwardEntry);

	FHBHeroSelectionEntry LockedFutureEntry;
	LockedFutureEntry.HeroClass = nullptr;
	LockedFutureEntry.HeroName = FText::FromString(TEXT("Future Hero"));
	LockedFutureEntry.RoleCategory = EHBHeroRoleCategory::Future;
	LockedFutureEntry.AbilityOneDescription = FText::FromString(TEXT("Coming soon."));
	LockedFutureEntry.AbilityTwoDescription = FText::FromString(TEXT("Coming soon."));
	LockedFutureEntry.AbilityThreeDescription = FText::FromString(TEXT("Coming soon."));
	LockedFutureEntry.bLocked = true;
	HeroEntries.Add(LockedFutureEntry);
}

void UHBHeroSelectionWidget::BuildRuntimeWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!WidgetTree->RootWidget)
	{
		UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HeroSelectionRoot"));
		WidgetTree->RootWidget = RootBox;

		HeroCatalogText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HeroCatalogText"));
		if (HeroCatalogText)
		{
			HeroCatalogText->SetAutoWrapText(true);
			RootBox->AddChild(HeroCatalogText);
		}

		HeroChoiceCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("HeroChoiceCombo"));
		if (HeroChoiceCombo)
		{
			RootBox->AddChild(HeroChoiceCombo);
		}

		ConfirmSelectionButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmSelectionButton"));
		if (ConfirmSelectionButton)
		{
			UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfirmSelectionLabel"));
			if (ButtonLabel)
			{
				ButtonLabel->SetText(FText::FromString(TEXT("Lock Hero Selection")));
				ConfirmSelectionButton->AddChild(ButtonLabel);
			}

			RootBox->AddChild(ConfirmSelectionButton);
		}

		return;
	}

	HeroCatalogText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("HeroCatalogText")));
	HeroChoiceCombo = Cast<UComboBoxString>(WidgetTree->FindWidget(TEXT("HeroChoiceCombo")));
	ConfirmSelectionButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("ConfirmSelectionButton")));
}

void UHBHeroSelectionWidget::RefreshRuntimeText()
{
	if (!HeroCatalogText)
	{
		return;
	}

	FString CatalogString = TEXT("Hero Selection\n");
	CatalogString += FString::Printf(TEXT("Requested Team: %s\n\n"), RequestedTeam == ETeam::TeamB ? TEXT("Team B") : TEXT("Team A"));

	for (int32 Index = 0; Index < HeroEntries.Num(); ++Index)
	{
		const FHBHeroSelectionEntry& Entry = HeroEntries[Index];
		const FString LockText = Entry.bLocked ? TEXT("[LOCKED]") : TEXT("[AVAILABLE]");
		CatalogString += FString::Printf(
			TEXT("%d) %s  %s\nRole: %s\n- %s\n- %s\n- %s\n\n"),
			Index + 1,
			*Entry.HeroName.ToString(),
			*LockText,
			*UEnum::GetValueAsString(Entry.RoleCategory),
			*Entry.AbilityOneDescription.ToString(),
			*Entry.AbilityTwoDescription.ToString(),
			*Entry.AbilityThreeDescription.ToString());
	}

	if (*SelectedHeroClass)
	{
		CatalogString += FString::Printf(TEXT("Selected Hero: %s\n"), *SelectedHeroClass->GetName());
	}
	else
	{
		CatalogString += TEXT("Selected Hero: None\n");
	}

	HeroCatalogText->SetText(FText::FromString(CatalogString));
}

void UHBHeroSelectionWidget::RefreshHeroChoiceOptions()
{
	if (!HeroChoiceCombo)
	{
		return;
	}

	HeroChoiceCombo->ClearOptions();
	for (const FHBHeroSelectionEntry& Entry : HeroEntries)
	{
		HeroChoiceCombo->AddOption(Entry.HeroName.ToString());
	}

	if (HeroEntries.Num() > 0)
	{
		HeroChoiceCombo->SetSelectedOption(HeroEntries[0].HeroName.ToString());
	}
}

const FHBHeroSelectionEntry* UHBHeroSelectionWidget::FindHeroEntry(TSubclassOf<APawn> HeroClass) const
{
	for (const FHBHeroSelectionEntry& Entry : HeroEntries)
	{
		if (Entry.HeroClass == HeroClass)
		{
			return &Entry;
		}
	}

	return nullptr;
}

bool UHBHeroSelectionWidget::CanSubmitSelection() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const AHBSoccerGameState* SoccerGameState = World->GetGameState<AHBSoccerGameState>();
	if (!SoccerGameState)
	{
		return true;
	}

	return SoccerGameState->MatchPhase == EMatchPhase::None ||
		SoccerGameState->MatchPhase == EMatchPhase::Warmup ||
		SoccerGameState->MatchPhase == EMatchPhase::Countdown;
}

int32 UHBHeroSelectionWidget::GetSelectedHeroIndex() const
{
	if (!HeroChoiceCombo)
	{
		return INDEX_NONE;
	}

	const FString SelectedOption = HeroChoiceCombo->GetSelectedOption();
	for (int32 Index = 0; Index < HeroEntries.Num(); ++Index)
	{
		if (HeroEntries[Index].HeroName.ToString() == SelectedOption)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

void UHBHeroSelectionWidget::HandleConfirmSelectionClicked()
{
	const int32 SelectedIndex = GetSelectedHeroIndex();
	if (!HeroEntries.IsValidIndex(SelectedIndex))
	{
		return;
	}

	const FHBHeroSelectionEntry& Entry = HeroEntries[SelectedIndex];
	SelectHeroAndTeam(Entry.HeroClass, RequestedTeam);
}
