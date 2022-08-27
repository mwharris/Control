#include "TelekineticActor.h"
#include "TelekinesisCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

ATelekineticActor::ATelekineticActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	TelekineticMesh = CreateOptionalDefaultSubobject<UStaticMeshComponent>("Telekinetic Mesh");
	if (TelekineticMesh)
	{
		SetRootComponent(TelekineticMesh);
	}
}

void ATelekineticActor::BeginPlay()
{
	Super::BeginPlay();
}

void ATelekineticActor::Highlight(bool bHighlight)
{
	TelekineticMesh->SetRenderCustomDepth(bHighlight);
}

void ATelekineticActor::Pull(ATelekinesisCharacter* InPlayerCharacter)
{
	PlayerCharacter = InPlayerCharacter;
	StartLift();
}

void ATelekineticActor::Push(FVector Destination)
{
	// TODO: ...
}

void ATelekineticActor::StartLift()
{
	Highlight(false);
	LiftStart = GetActorLocation();
	LiftStartTimeSeconds = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().SetTimer(LiftTimerHandle, this, &ATelekineticActor::Lift, 0.016f, true);
}

void ATelekineticActor::Lift()
{
	// Determine our Alpha value
	const float CurrTimeSeconds = GetWorld()->GetTimeSeconds();
	const float Alpha = UKismetMathLibrary::MapRangeClamped(CurrTimeSeconds, LiftStartTimeSeconds, GetLiftEndTimeSeconds(), 0.f, 1.0f);
	// Move upwards, relative to our start location, equal to our LiftHeight
	const FVector TargetLocation = LiftStart + FVector(0.f, 0.f, LiftStart.Z + LiftHeight);
	// Finished, jump to end location
	if (CurrTimeSeconds >= GetLiftEndTimeSeconds())
	{
		SetActorLocation(TargetLocation);
		StartReach();
		GetWorldTimerManager().ClearTimer(LiftTimerHandle);
		LiftTimerHandle.Invalidate();
	}
	// Otherwise, Lerp from our current location to our target location
	else
	{
		SetActorLocation(UKismetMathLibrary::VLerp(LiftStart, TargetLocation, Alpha));	
	}
}

void ATelekineticActor::StartReach()
{
	TelekineticMesh->SetEnableGravity(false);
	TelekineticMesh->SetLinearDamping(20.0f);
	TelekinesisState = ETelekinesisStates::Pulled;
	GetWorldTimerManager().SetTimer(ReachTimerHandle, this, &ATelekineticActor::ReachCharacter, 0.016, true);
}

void ATelekineticActor::ReachCharacter() const
{
	if (PlayerCharacter == nullptr)
	{
		return;
	}
	// Get the direction we want to move, make sure we don't move too fast
	FVector MoveDirection = PlayerCharacter->GetTelekineticPropLocation() - GetActorLocation();
	MoveDirection = UKismetMathLibrary::ClampVectorSize(MoveDirection, 0.f, 1000.f);
	// Lighter objects should move faster, heavier objects should move slower
	MoveDirection *= UKismetMathLibrary::MapRangeClamped(
		TelekineticMesh->GetMass(),
		MassMinRange,
		MassMaxRange,
		MassMultiplierMaxRange,
		MassMultiplierMinRange
	);
	// Add an impulse to our object to move it towards the player
	TelekineticMesh->AddImpulse(MoveDirection, NAME_None, true);
}

float ATelekineticActor::GetLiftEndTimeSeconds() const
{
	return LiftStartTimeSeconds + LiftDurationSeconds;
}
