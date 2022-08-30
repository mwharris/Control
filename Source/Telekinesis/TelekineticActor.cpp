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

	TelekineticMesh->OnComponentHit.AddDynamic(this, &ATelekineticActor::OnHitCallback);
}

void ATelekineticActor::BeginPlay()
{
	Super::BeginPlay();
}

void ATelekineticActor::Pull(ATelekinesisCharacter* InPlayerCharacter)
{
	PlayerCharacter = InPlayerCharacter;
	StartLift();
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
	TelekinesisState = ETelekinesisStates::Pulled;

	// Determine our Alpha value
	const float CurrTimeSeconds = GetWorld()->GetTimeSeconds();
	const float Alpha = UKismetMathLibrary::MapRangeClamped(CurrTimeSeconds, LiftStartTimeSeconds, GetLiftEndTimeSeconds(), 0.f, 1.0f);

	// Move upwards, relative to our start location, equal to our LiftHeight
	const float TargetHeight = LiftStart.Z + LiftHeight;
	const float NewHeight = UKismetMathLibrary::Lerp(GetActorLocation().Z, TargetHeight, Alpha);
	
	// Start Reach before we're fully done for a smoother transition between the two phases
	if (Alpha >= LiftReachTransitionPercent && !ReachTimerHandle.IsValid())
	{
		// Reach our Player's TK Prop hold location
		StartReach(true);
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

void ATelekineticActor::Push(FVector Destination)
{
	TelekinesisState = ETelekinesisStates::Pushed;
	PushDestination = Destination;
	PushDirection = (Destination - GetActorLocation()).GetSafeNormal();
	// Stop our Lift timer if that's active
	if (LiftTimerHandle.IsValid())
	{
		GetWorldTimerManager().ClearTimer(LiftTimerHandle);
		LiftTimerHandle.Invalidate();
	}
	// If our Reach timer has already begun, reset it with a new target
	if (ReachTimerHandle.IsValid())
	{
		ClearReachTimer();
	}
	// Call reach with the passed-in destination
	ReachTarget = Destination;
	StartReach(false);
}

void ATelekineticActor::StartReach(bool bReachCharacter)
{
	TelekineticMesh->SetEnableGravity(false);
	TelekineticMesh->SetLinearDamping(20.0f);
	JitterFrameTime = UKismetMathLibrary::RandomIntegerInRange(JitterFrameTimeRangeMin, JitterFrameTimeRangeMax);
	// Add a random angular impulse so the object isn't so static
	const float ImpulseStrength = UKismetMathLibrary::RandomFloatInRange(LiftAngularImpulseMinStrength, LiftAngularImpulseMaxStrength);
	const FVector AngularImpulse = UKismetMathLibrary::RandomUnitVector() * ImpulseStrength;
	TelekineticMesh->AddAngularImpulseInDegrees(AngularImpulse, NAME_None, true);
	// Reach Character or Target depending on boolean
	if (bReachCharacter)
	{
		GetWorldTimerManager().SetTimer(ReachTimerHandle, this, &ATelekineticActor::ReachCharacter, 0.016, true);
	}
	else
	{
		GetWorldTimerManager().SetTimer(ReachTimerHandle, this, &ATelekineticActor::ReachPoint, 0.016, true);
	}
}

void ATelekineticActor::ReachCharacter()
{
	if (PlayerCharacter == nullptr)
	{
		return;
	}
	ReachLocation(PlayerCharacter->GetTelekineticPropLocation(), PullSpeedMultiplier, false);
}

void ATelekineticActor::ReachPoint()
{
	ReachLocation(ReachTarget, PushSpeedMultiplier, true);
}

void ATelekineticActor::ReachLocation(const FVector& Location, float SpeedMultiplier, bool bConstantSpeed)
{
	// Get the direction we want to move
	FVector MoveDirection = Location - GetActorLocation();
	// Pull at a constant rate, not based on distance
	if (bConstantSpeed)
	{
		MoveDirection = MoveDirection.GetSafeNormal();
	}
	// Make sure we don't pull/push too fast
	MoveDirection = UKismetMathLibrary::ClampVectorSize(MoveDirection, 0.f, 1000.f);
	// Lighter objects should move faster, heavier objects should move slower
	MoveDirection *= UKismetMathLibrary::MapRangeClamped(
		TelekineticMesh->GetMass(),
		MassMinRange,
		MassMaxRange,
		MassMultiplierMaxRange,
		MassMultiplierMinRange
	);
	// Add any additional speed multiplier we need
	MoveDirection *= SpeedMultiplier;
	// Add an impulse to our object to reach its destination
	TelekineticMesh->AddImpulse(MoveDirection, NAME_None, true);
	// Jitter the object periodically while it's held
	if (TelekinesisState == ETelekinesisStates::Pulled)
	{
		Jitter();
	}
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

void ATelekineticActor::OnHitCallback(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (TelekinesisState != ETelekinesisStates::Pushed)
	{
		return;
	}
	// Reset variables updated when we lift/reach
	TelekineticMesh->SetEnableGravity(true);
	TelekineticMesh->SetLinearDamping(0.1f);
	TelekinesisState = ETelekinesisStates::Default;
	ClearReachTimer();

	// Add our own slight bounce impulse
	const FVector Reflection = UKismetMathLibrary::GetReflectionVector(PushDirection, Hit.ImpactNormal);
	UKismetSystemLibrary::DrawDebugLine(
		GetWorld(),
		Hit.ImpactPoint, 
		Hit.ImpactPoint + (Reflection * 1000.f),
		FLinearColor::White,
		10.f,
		5.f
	);
	// Reduce the physic's engine influence but attempt to keep the direction
	TelekineticMesh->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
	TelekineticMesh->AddImpulse(Hit.ImpactPoint + (Reflection * CollisionBounciness));
}

void ATelekineticActor::Highlight(bool bHighlight)
{
	TelekineticMesh->SetRenderCustomDepth(bHighlight);
}

float ATelekineticActor::GetLiftEndTimeSeconds() const
{
	return LiftStartTimeSeconds + LiftDurationSeconds;
}

void ATelekineticActor::ClearReachTimer()
{
	GetWorldTimerManager().ClearTimer(ReachTimerHandle);
	ReachTimerHandle.Invalidate();
}