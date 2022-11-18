#include "TelekineticActor.h"
#include "TelekinesisCharacter.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MiniTelekineticActor.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

ATelekineticActor::ATelekineticActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Setup Mesh and OnComponentHit callback
	TelekineticMesh = CreateOptionalDefaultSubobject<UStaticMeshComponent>("Telekinetic Mesh");
	if (TelekineticMesh)
	{
		SetRootComponent(TelekineticMesh);
	}
	TelekineticMesh->OnComponentHit.AddDynamic(this, &ATelekineticActor::OnHitCallback);

	// Setup the Attraction Field for Mini Props
	AttractionField = CreateDefaultSubobject<USphereComponent>("Attraction Field");
	AttractionField->SetupAttachment(RootComponent);
	AttractionField->OnComponentBeginOverlap.AddDynamic(this, &ATelekineticActor::OnBeginOverlap);
	AttractionField->OnComponentEndOverlap.AddDynamic(this, &ATelekineticActor::OnEndOverlap);

	// AudioComponent for wind sound
	AudioComponent = CreateDefaultSubobject<UAudioComponent>("Wind");
	AudioComponent->SetupAttachment(RootComponent);
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
	ActivateParticleSystem();
	DetectMiniProps();
	UGameplayStatics::PlaySound2D(GetWorld(), LiftSound);
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
	PushDirection = (Destination - GetActorLocation()).GetSafeNormal();
	// Disable mini props collision so we don't collide with attracted mini props
	for (const auto MiniProp : AttractedMiniProps)
	{
		MiniProp->GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
	}
	// Release all attracted mini props
	AttractedMiniProps.Empty();
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
	UGameplayStatics::PlaySound2D(GetWorld(), PushSound);
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
		AudioComponent->Activate(true);
		GetWorldTimerManager().SetTimer(ReachTimerHandle, this, &ATelekineticActor::ReachCharacter, 0.0167, true);
	}
	else
	{
		AudioComponent->Deactivate();
		GetWorldTimerManager().SetTimer(ReachTimerHandle, this, &ATelekineticActor::ReachPoint, 0.0167, true);
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
	FeedLocationToParticleSystem();
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
	// Jitter and attract MiniProps while an object is held
	if (TelekinesisState == ETelekinesisStates::Pulled)
	{
		Jitter();
		AttractMiniProps();
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
	// Deactivate the held particle system
	DeactivateParticleSystem();
	// Reset variables updated when we lift/reach
	TelekineticMesh->SetEnableGravity(true);
	TelekineticMesh->SetLinearDamping(0.1f);
	TelekinesisState = ETelekinesisStates::Default;
	ClearReachTimer();
	// Add our own slight bounce impulse
	const FVector Reflection = UKismetMathLibrary::GetReflectionVector(PushDirection, Hit.ImpactNormal);
	// Reduce the physic's engine influence but attempt to keep the direction
	TelekineticMesh->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
	TelekineticMesh->AddImpulse(Hit.ImpactPoint + (Reflection * CollisionBounciness));
	// Spawn sparks particle system (blueprints)
	SpawnSparks(-PushDirection);
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

void ATelekineticActor::AttractMiniProps()
{
	for (const auto MiniProp : AttractedMiniProps)
	{
		FVector Direction = (GetActorLocation() - MiniProp->GetActorLocation()).GetSafeNormal();
		MiniProp->AttractForce(Direction);
	}
}

void ATelekineticActor::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                       int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AMiniTelekineticActor* MiniProp = Cast<AMiniTelekineticActor>(OtherActor);
	if (MiniProp == nullptr)
	{
		return;
	}
	AddMiniProp(MiniProp);
}

void ATelekineticActor::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	AMiniTelekineticActor* MiniProp = Cast<AMiniTelekineticActor>(OtherActor);
	if (MiniProp == nullptr)
	{
		return;
	}
	RemoveMiniProp(MiniProp);
}

// Detect any MiniProps that are in the immediate radius of a Lifted object
void ATelekineticActor::DetectMiniProps()
{
	// Setup variables for the sphere trace
	const TArray<AActor*> ActorsToIgnore;
	TArray<FHitResult> HitResults;

	// Only look for MiniProp object types
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(ObjectTypeQuery2);

	// Perform the trace
	UKismetSystemLibrary::SphereTraceMultiForObjects(
		GetWorld(),
		GetActorLocation(),
		GetActorLocation(),
		AttractionField->GetScaledSphereRadius(),
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		HitResults,
		true
	);

	// Add any hit mini props to the attracted mini props array
	for (auto Hit : HitResults)
	{
		AMiniTelekineticActor* MiniProp = Cast<AMiniTelekineticActor>(Hit.GetActor());
		if (MiniProp == nullptr)
		{
			continue;
		}
		AddMiniProp(MiniProp);
	}
}

void ATelekineticActor::AddMiniProp(AMiniTelekineticActor* MiniProp)
{
	AttractedMiniProps.Add(MiniProp);
	MiniProp->GetMesh()->SetEnableGravity(false);
	MiniProp->GetMesh()->SetLinearDamping(10.f);
	// Make sure MiniProps collide with the held object
	MiniProp->GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);
}

void ATelekineticActor::RemoveMiniProp(AMiniTelekineticActor* MiniProp)
{
	AttractedMiniProps.Remove(MiniProp);
	MiniProp->GetMesh()->SetEnableGravity(true);
	MiniProp->GetMesh()->SetLinearDamping(0.01f);
}