#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoccerTypes.h"
#include "HBSoccerBall.generated.h"

class AHBGoalTriggerVolume;
class APawn;
class UPrimitiveComponent;
class USphereComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBallPossessionChanged, APawn*, PreviousPawn, APawn*, NewPawn);

UCLASS()
class MYPROJECT_API AHBSoccerBall : public AActor
{
	GENERATED_BODY()

public:
	AHBSoccerBall();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball")
	TObjectPtr<USphereComponent> BallCollision = nullptr;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PossessingPawn, Category = "Ball")
	TObjectPtr<APawn> PossessingPawn = nullptr;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Ball")
	ETeam LastTouchTeam = ETeam::None;

	UPROPERTY(BlueprintAssignable, Category = "Ball")
	FOnBallPossessionChanged OnBallPossessionChanged;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ball")
	float MaxKickDistance = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ball")
	float MaxImpulseStrength = 7000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ball")
	float PossessionStealDistance = 240.0f;

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void KickBall(APawn* Kicker, FVector Impulse);

	UFUNCTION(BlueprintCallable, Category = "Ball")
	bool TryKickFromCharacter(APawn* Kicker, FVector DesiredDirection, float DesiredImpulseStrength);

	UFUNCTION(BlueprintCallable, Category = "Ball")
	bool TryStealPossession(APawn* Stealer);

	void ServerResetBallToCenter();

	UFUNCTION(BlueprintPure, Category = "Ball")
	APawn* GetPossessingPawn() const { return PossessingPawn; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	FTransform InitialBallTransform;

	bool bGoalProcessing = false;

	UFUNCTION()
	void OnBallBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnBallEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UFUNCTION()
	void OnRep_PossessingPawn(APawn* PreviousPawn);

	void SetPossessingPawn(APawn* NewPawn);
	void HandleGoalOverlap(const AHBGoalTriggerVolume* GoalVolume);
	void ResetBallToCenter();
	bool IsCharacterInRange(const APawn* Character, float InteractionRange) const;
	ETeam ResolveTeamFromPawn(const APawn* Pawn) const;
};
