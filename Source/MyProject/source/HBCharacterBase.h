#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SoccerTypes.h"
#include "HBCharacterBase.generated.h"

class AHBSoccerBall;
class UHBAttributeComponent;

UCLASS()
class MYPROJECT_API AHBCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AHBCharacterBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHBAttributeComponent> AttributeComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Team, Category = "Team")
	ETeam Team = ETeam::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ball Interaction")
	float KickImpulseBase = 2200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ball Interaction")
	float PassImpulseBase = 1800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ball Interaction")
	float ChargedShotImpulseBase = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ball Interaction")
	float ChargedShotMaxMultiplier = 2.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ball Interaction")
	float MinChargedShotTime = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ball Interaction")
	float MaxChargedShotTime = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ball Interaction")
	float PassMaxDistance = 3000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooldowns")
	float KickCooldown = 0.20f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooldowns")
	float PassCooldown = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooldowns")
	float ChargedShotCooldown = 1.20f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooldowns")
	float StealCooldown = 0.55f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Cooldowns")
	float NextKickAllowedServerTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Cooldowns")
	float NextPassAllowedServerTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Cooldowns")
	float NextChargedShotAllowedServerTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Cooldowns")
	float NextStealAllowedServerTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Ball Interaction")
	bool bIsChargingShot = false;

	UFUNCTION(BlueprintPure, Category = "Attributes")
	UHBAttributeComponent* GetAttributeComponent() const { return AttributeComponent; }

	UFUNCTION(BlueprintCallable, Category = "Ball Interaction")
	void KickSoccerBall(AHBSoccerBall* Ball, FVector KickDirection);

	UFUNCTION(BlueprintCallable, Category = "Ball Interaction")
	void PassSoccerBall(AHBSoccerBall* Ball, AHBCharacterBase* TargetTeammate);

	UFUNCTION(BlueprintCallable, Category = "Ball Interaction")
	void StartChargedShot();

	UFUNCTION(BlueprintCallable, Category = "Ball Interaction")
	void ReleaseChargedShot(AHBSoccerBall* Ball, FVector ShotDirection);

	UFUNCTION(BlueprintCallable, Category = "Ball Interaction")
	void StealBallPossession(AHBSoccerBall* Ball);

	UFUNCTION(Server, Reliable)
	void ServerKickSoccerBall(AHBSoccerBall* Ball, FVector_NetQuantizeNormal KickDirection);

	UFUNCTION(Server, Reliable)
	void ServerPassSoccerBall(AHBSoccerBall* Ball, AHBCharacterBase* TargetTeammate);

	UFUNCTION(Server, Reliable)
	void ServerStartChargedShot();

	UFUNCTION(Server, Reliable)
	void ServerReleaseChargedShot(AHBSoccerBall* Ball, FVector_NetQuantizeNormal ShotDirection);

	UFUNCTION(Server, Reliable)
	void ServerStealBallPossession(AHBSoccerBall* Ball);

	void SetTeam(ETeam NewTeam);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	float ChargedShotStartServerTime = -1.0f;

	UFUNCTION()
	void OnRep_Team();

	bool IsActionOffCooldown(float NextAllowedServerTime) const;
	float GetKickPowerMultiplier() const;
	float GetPassAccuracyAlpha() const;
	ETeam GetHeroTeam() const;
};
