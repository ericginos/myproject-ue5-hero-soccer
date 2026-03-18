#include "source/HBGoalTriggerVolume.h"

#include "Components/BoxComponent.h"

AHBGoalTriggerVolume::AHBGoalTriggerVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	GoalTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("GoalTrigger"));
	SetRootComponent(GoalTrigger);

	GoalTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GoalTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	GoalTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	GoalTrigger->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	GoalTrigger->SetGenerateOverlapEvents(true);
	GoalTrigger->SetBoxExtent(FVector(200.0f, 600.0f, 250.0f));
}
