#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "source/SoccerTypes.h"
#include "HBHeroSelectionWidget.generated.h"

class APawn;
class UButton;
class UComboBoxString;
class UTextBlock;
class UVerticalBox;

UENUM(BlueprintType)
enum class EHBHeroRoleCategory : uint8
{
	Goalie UMETA(DisplayName = "Goalie"),
	Defender UMETA(DisplayName = "Defender"),
	Midfielder UMETA(DisplayName = "Midfielder"),
	Forward UMETA(DisplayName = "Forward"),
	Future UMETA(DisplayName = "Future")
};

USTRUCT(BlueprintType)
struct FHBHeroSelectionEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero")
	TSubclassOf<APawn> HeroClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero")
	FText HeroName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero")
	EHBHeroRoleCategory RoleCategory = EHBHeroRoleCategory::Future;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero")
	FText AbilityOneDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero")
	FText AbilityTwoDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero")
	FText AbilityThreeDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero")
	bool bLocked = false;
};

UCLASS(BlueprintType, Blueprintable)
class MYPROJECT_API UHBHeroSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero Selection")
	TArray<FHBHeroSelectionEntry> HeroEntries;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero Selection")
	ETeam RequestedTeam = ETeam::TeamA;

	UPROPERTY(BlueprintReadOnly, Category = "Hero Selection")
	TSubclassOf<APawn> SelectedHeroClass;

	UFUNCTION(BlueprintCallable, Category = "Hero Selection")
	const TArray<FHBHeroSelectionEntry>& GetHeroEntries() const { return HeroEntries; }

	UFUNCTION(BlueprintCallable, Category = "Hero Selection")
	bool SelectHero(TSubclassOf<APawn> HeroClass);

	UFUNCTION(BlueprintCallable, Category = "Hero Selection")
	bool SelectHeroAndTeam(TSubclassOf<APawn> HeroClass, ETeam TeamToJoin);

	UFUNCTION(BlueprintPure, Category = "Hero Selection")
	bool IsHeroLocked(TSubclassOf<APawn> HeroClass) const;

private:
	void BuildDefaultHeroEntries();
	void BuildRuntimeWidgetTree();
	void RefreshRuntimeText();
	void RefreshHeroChoiceOptions();
	const FHBHeroSelectionEntry* FindHeroEntry(TSubclassOf<APawn> HeroClass) const;
	bool CanSubmitSelection() const;
	int32 GetSelectedHeroIndex() const;

	UFUNCTION()
	void HandleConfirmSelectionClicked();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HeroCatalogText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> HeroChoiceCombo = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ConfirmSelectionButton = nullptr;
};
