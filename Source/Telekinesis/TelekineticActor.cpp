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
	const float TargetHeight = LiftStart.Z + LiftHeight;
	const float NewHeight = UKismetMathLibrary::Lerp(GetActorLocation().Z, TargetHeight, Alpha);
	
	// Start Reach before we're fully done for a smoother transition between the two phases
	if (Alpha >= LiftReachTransitionPercent && !ReachTimerHandle.IsValid())
	{
		StartReach();
	}
	
	// Finished, jump to end location
	if (CurrTimeSeconds >= GetLiftEndTimeSeconds())
	{
		SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, TargetHeight));	
		GetWorldTimerManager().ClearTimer(LiftTimerHandle);
		LiftTimerHandle.Invalidate();
	}
	// Otherwise, Lerp from our current location to our target location
	else
	{
		SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, NewHeight));	
	}
}

void ATelekineticActor::StartReach()
{
	TelekineticMesh->SetEnableGravity(false);
	TelekineticMesh->SetLinearDamping(20.0f);
	TelekinesisState = ETelekinesisStates::Pulled;
	JitterFrameTime = UKismetMathLibrary::RandomIntegerInRange(JitterFrameTimeRangeMin, JitterFrameTimeRangeMax);
	// Add a random angular impulse so the object isn't so static
	const float ImpulseStrength = UKismetMathLibrary::RandomFloatInRange(LiftAngularImpulseMinStrength, LiftAngularImpulseMaxStrength);
	const FVector AngularImpulse = UKismetMathLibrary::RandomUnitVector() * ImpulseStrength;
	TelekineticMesh->AddAngularImpulseInDegrees(AngularImpulse, NAME_None, true);
	// Start pulling our object towards the player over time
	GetWorldTimerManager().SetTimer(ReachTimerHandle, this, &ATelekineticActor::ReachCharacter, 0.016, true);
}

void ATelekineticActor::ReachCharacter()
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
	// Jitter the object periodically while it's held
	Jitter();
}

void ATelekineticActor::Jitter()
{
	JitterCounter++;
	if (JitterCounter < JitterFrameTime)
	{
		return;
	}
	JitterCounter = 0;
	JitterFrameTime = UKismetMathLibrary::RandomIntegerInRange(JitterFrameTimeRangeMin, JitterFrameTimeRangeMax);
	const int32 Strength = UKismetMathLibrary::RandomIntegerInRange(JitterStrengthMinMultiplier, JitterStrengthMaxMultiplier);
	TelekineticMesh->AddImpulse(UKismetMathLibrary::RandomUnitVector() * Strength, NAME_None, true);
}

float ATelekineticActor::GetLiftEndTimeSeconds() const
{
	return LiftStartTimeSeconds + LiftDurationSeconds;
}
