#include "source/HBSoccerBall.h"

#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "source/HBGoalTriggerVolume.h"
#include "source/HBSoccerGameMode.h"
#include "source/HBSoccerPlayerState.h"

AHBSoccerBall::AHBSoccerBall()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = false;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(90.0f);
	SetMinNetUpdateFrequency(30.0f);
	NetPriority = 3.0f;
	SetNetCullDistanceSquared(FMath::Square(25000.0f));

	BallCollision = CreateDefaultSubobject<USphereComponent>(TEXT("BallCollision"));
	SetRootComponent(BallCollision);

	BallCollision->SetSphereRadius(22.0f);
	BallCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BallCollision->SetCollisionObjectType(ECC_PhysicsBody);
	BallCollision->SetCollisionResponseToAllChannels(ECR_Block);
	BallCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BallCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	BallCollision->SetGenerateOverlapEvents(true);
	BallCollision->SetSimulatePhysics(true);
	BallCollision->SetIsReplicated(true);
	BallCollision->SetEnableGravity(true);
	BallCollision->SetNotifyRigidBodyCollision(true);
}

void AHBSoccerBall::BeginPlay()
{
	Super::BeginPlay();

	InitialBallTransform = GetActorTransform();

	if (BallCollision)
	{
		BallCollision->OnComponentBeginOverlap.AddDynamic(this, &AHBSoccerBall::OnBallBeginOverlap);
		BallCollision->OnComponentEndOverlap.AddDynamic(this, &AHBSoccerBall::OnBallEndOverlap);
	}
}

bool AHBSoccerBall::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	if (PossessingPawn)
	{
		if (ViewTarget == PossessingPawn || RealViewer == PossessingPawn)
		{
			return true;
		}

		if ((ViewTarget && PossessingPawn->IsOwnedBy(ViewTarget)) || (RealViewer && PossessingPawn->IsOwnedBy(RealViewer)))
		{
			return true;
		}
	}

	const AActor* EffectiveViewer = ViewTarget ? ViewTarget : RealViewer;
	if (EffectiveViewer)
	{
		const float DistanceSq = FVector::DistSquared(EffectiveViewer->GetActorLocation(), GetActorLocation());
		if (DistanceSq <= GetNetCullDistanceSquared())
		{
			return true;
		}
	}

	return Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
}

void AHBSoccerBall::KickBall(APawn* Kicker, FVector Impulse)
{
	if (!Kicker)
	{
		return;
	}

	const FVector Direction = Impulse.IsNearlyZero() ? Kicker->GetActorForwardVector() : Impulse.GetSafeNormal();
	const float Strength = Impulse.Size();
	TryKickFromCharacter(Kicker, Direction, Strength);
}

bool AHBSoccerBall::TryKickFromCharacter(APawn* Kicker, FVector DesiredDirection, float DesiredImpulseStrength)
{
	if (!HasAuthority() || !Kicker || !BallCollision)
	{
		return false;
	}

	if (!IsCharacterInRange(Kicker, MaxKickDistance))
	{
		return false;
	}

	FVector KickDirection = DesiredDirection.GetSafeNormal();
	if (KickDirection.IsNearlyZero())
	{
		KickDirection = Kicker->GetActorForwardVector();
	}

	const float KickStrength = FMath::Clamp(DesiredImpulseStrength, 0.0f, MaxImpulseStrength);
	if (KickStrength <= 0.0f)
	{
		return false;
	}

	SetPossessingPawn(Kicker);
	LastTouchTeam = ResolveTeamFromPawn(Kicker);

	BallCollision->AddImpulse(KickDirection * KickStrength, NAME_None, true);
	return true;
}

bool AHBSoccerBall::TryStealPossession(APawn* Stealer)
{
	if (!HasAuthority() || !Stealer || !BallCollision)
	{
		return false;
	}

	if (!IsCharacterInRange(Stealer, PossessionStealDistance))
	{
		return false;
	}

	if (PossessingPawn && PossessingPawn != Stealer)
	{
		const ETeam PossessorTeam = ResolveTeamFromPawn(PossessingPawn);
		const ETeam StealerTeam = ResolveTeamFromPawn(Stealer);
		if (PossessorTeam != ETeam::None && PossessorTeam == StealerTeam)
		{
			return false;
		}
	}

	SetPossessingPawn(Stealer);
	LastTouchTeam = ResolveTeamFromPawn(Stealer);

	BallCollision->SetPhysicsLinearVelocity(BallCollision->GetPhysicsLinearVelocity() * 0.4f);
	BallCollision->SetPhysicsAngularVelocityInDegrees(BallCollision->GetPhysicsAngularVelocityInDegrees() * 0.4f);
	return true;
}

void AHBSoccerBall::ServerResetBallToCenter()
{
	ResetBallToCenter();
}

void AHBSoccerBall::OnBallBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority() || !OtherActor)
	{
		return;
	}

	if (APawn* OverlappingPawn = Cast<APawn>(OtherActor))
	{
		if (!PossessingPawn)
		{
			SetPossessingPawn(OverlappingPawn);
			LastTouchTeam = ResolveTeamFromPawn(OverlappingPawn);
		}
		return;
	}

	if (const AHBGoalTriggerVolume* GoalVolume = Cast<AHBGoalTriggerVolume>(OtherActor))
	{
		HandleGoalOverlap(GoalVolume);
	}
}

void AHBSoccerBall::OnBallEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	APawn* EndingPawn = Cast<APawn>(OtherActor);
	if (EndingPawn && PossessingPawn == EndingPawn)
	{
		SetPossessingPawn(nullptr);
	}
}

void AHBSoccerBall::OnRep_PossessingPawn(APawn* PreviousPawn)
{
	OnBallPossessionChanged.Broadcast(PreviousPawn, PossessingPawn);
}

void AHBSoccerBall::SetPossessingPawn(APawn* NewPawn)
{
	if (!HasAuthority() || PossessingPawn == NewPawn)
	{
		return;
	}

	APawn* PreviousPawn = PossessingPawn;
	PossessingPawn = NewPawn;
	OnRep_PossessingPawn(PreviousPawn);
}

void AHBSoccerBall::HandleGoalOverlap(const AHBGoalTriggerVolume* GoalVolume)
{
	if (!HasAuthority() || !GoalVolume || bGoalProcessing)
	{
		return;
	}

	bGoalProcessing = true;

	if (AHBSoccerGameMode* SoccerGameMode = Cast<AHBSoccerGameMode>(GetWorld()->GetAuthGameMode()))
	{
		const bool bGoalAccepted = SoccerGameMode->HandleGoalScored(this, GoalVolume->GoalTeam);
		if (!bGoalAccepted)
		{
			bGoalProcessing = false;
		}
	}
	else
	{
		bGoalProcessing = false;
	}
}

void AHBSoccerBall::ResetBallToCenter()
{
	if (!HasAuthority() || !BallCollision)
	{
		return;
	}

	SetActorTransform(InitialBallTransform, false, nullptr, ETeleportType::TeleportPhysics);
	BallCollision->SetPhysicsLinearVelocity(FVector::ZeroVector);
	BallCollision->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

	SetPossessingPawn(nullptr);
	LastTouchTeam = ETeam::None;
	bGoalProcessing = false;
}

bool AHBSoccerBall::IsCharacterInRange(const APawn* Character, float InteractionRange) const
{
	if (!Character)
	{
		return false;
	}

	return FVector::Dist(Character->GetActorLocation(), GetActorLocation()) <= InteractionRange;
}

ETeam AHBSoccerBall::ResolveTeamFromPawn(const APawn* Pawn) const
{
	if (!Pawn)
	{
		return ETeam::None;
	}

	const AHBSoccerPlayerState* SoccerPlayerState = Pawn->GetPlayerState<AHBSoccerPlayerState>();
	return SoccerPlayerState ? SoccerPlayerState->Team : ETeam::None;
}

void AHBSoccerBall::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHBSoccerBall, PossessingPawn);
	DOREPLIFETIME(AHBSoccerBall, LastTouchTeam);
}
