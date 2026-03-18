#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GoalShieldBarrier.generated.h"

class UBoxComponent;

UCLASS()
class MYPROJECT_API AGoalShieldBarrier : public AActor
{
	GENERATED_BODY()

public:
	AGoalShieldBarrier();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shield")
	TObjectPtr<UBoxComponent> BarrierCollision = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shield")
	FVector BarrierExtent = FVector(70.0f, 260.0f, 140.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shield")
	float DefaultLifeSeconds = 2.5f;

	UFUNCTION(BlueprintCallable, Category = "Shield")
	void ActivateBarrier(float LifetimeOverrideSeconds = -1.0f);
};
