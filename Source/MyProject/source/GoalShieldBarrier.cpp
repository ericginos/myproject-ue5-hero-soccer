#include "source/GoalShieldBarrier.h"

#include "Components/BoxComponent.h"

AGoalShieldBarrier::AGoalShieldBarrier()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = false;
	bNetUseOwnerRelevancy = true;
	SetReplicateMovement(false);
	SetNetUpdateFrequency(8.0f);
	SetMinNetUpdateFrequency(2.0f);
	NetPriority = 1.4f;
	SetNetCullDistanceSquared(FMath::Square(12000.0f));

	BarrierCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BarrierCollision"));
	SetRootComponent(BarrierCollision);

	BarrierCollision->SetBoxExtent(BarrierExtent);
	BarrierCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BarrierCollision->SetCollisionObjectType(ECC_WorldDynamic);
	BarrierCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BarrierCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	BarrierCollision->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	BarrierCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	BarrierCollision->SetCanEverAffectNavigation(false);
}

void AGoalShieldBarrier::ActivateBarrier(float LifetimeOverrideSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	const float Lifetime = LifetimeOverrideSeconds > 0.0f ? LifetimeOverrideSeconds : DefaultLifeSeconds;
	SetLifeSpan(FMath::Max(0.01f, Lifetime));
}
