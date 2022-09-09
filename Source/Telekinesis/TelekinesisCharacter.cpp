// Copyright Epic Games, Inc. All Rights Reserved.

#include "TelekinesisCharacter.h"

#include "TelekineticActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

//////////////////////////////////////////////////////////////////////////
// ATelekinesisCharacter

ATelekinesisCharacter::ATelekinesisCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rate for input
	TurnRateGamepad = 50.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = CameraDefaultArmLength;	
	CameraBoom->SetRelativeLocation(CameraDefaultLocation);
	CameraBoom->bUsePawnControlRotation = true;
	AddCameraBoomOffset();
	
	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create the Telekinetic Prop scene component
	PropSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("PropSceneComponent"));
	PropSceneComponent->SetupAttachment(GetMesh());

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void ATelekinesisCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("Telekinesis", IE_Pressed, this, &ATelekinesisCharacter::InputTelekinesis);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &ATelekinesisCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &ATelekinesisCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turn rate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &ATelekinesisCharacter::TurnRight);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
}

void ATelekinesisCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Only look for telekinesis objects
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(ObjectTypeQuery7);

	// Determine trace start location, offset by the detection radius so trace doesn't start behind the camera
	FVector StartLocation = FollowCamera->GetComponentLocation() + (FollowCamera->GetForwardVector() * DetectionRadius);
	// Determine trace end location
	FVector EndLocation = FollowCamera->GetComponentLocation() + (FollowCamera->GetForwardVector() * TelekinesisDistance);

	// Perform the trace
	FHitResult Hit;
	TArray<AActor*> ActorsToIgnore;
	UKismetSystemLibrary::SphereTraceSingleForObjects(
		GetWorld(),
		StartLocation,
		EndLocation,
		DetectionRadius,
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		Hit,
		true
	);

	// Check if we hit something
	if (Hit.GetActor() != nullptr)
	{
		ATelekineticActor* TKProp = Cast<ATelekineticActor>(Hit.GetActor());
		// Remove the highlight from any past TelekineticActor
		if (TelekineticTarget != nullptr)
		{
			TelekineticTarget->Highlight(false);
		}
		// Highlight and cache the new TelekineticActor
		TelekineticTarget = TKProp;
		TelekineticTarget->Highlight(true);
	}
	// Didn't hit anything, remove any active TelekineticActor
	else if (TelekineticTarget != nullptr)
	{
		TelekineticTarget->Highlight(false);
		TelekineticTarget = nullptr;
	}
}

void ATelekinesisCharacter::TurnRight(float Rate)
{
	AddControllerYawInput(Rate);
	// Rotate the actor with the camera if we're in Telekinesis mode.
	// This is so the player is always facing forward in Telekinesis mode.
	if (bTelekinesis && bFaceForward)
	{
		FRotator NewRotation = GetActorRotation();
		NewRotation.Yaw = GetControlRotation().Yaw;
		SetActorRotation(NewRotation);
	}
}

void ATelekinesisCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ATelekinesisCharacter::MoveRight(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void ATelekinesisCharacter::InputTelekinesis()
{
	// Pull or Push depending on our current state
	if (!bTelekinesis && TelekineticTarget != nullptr)
	{
		Pull();
	}
	else if (bTelekinesis)
	{
		Push();
	}
}

void ATelekinesisCharacter::Pull()
{
	bTelekinesis = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	CameraOffsetRightTarget = CameraZoomOffsetRight;
	CameraOffsetUpTarget = CameraZoomOffsetUp;
	CameraArmLengthTarget = CameraZoomArmLength;
	// Call blueprints to handle rotating / zooming in the camera
	Zoom();
	// Tell prop to lift and pull towards us
	CurrTelekineticProp = TelekineticTarget;
	TelekineticTarget->Pull(this);
	// Play our Pull animation
	PlayAnimMontage(PullAnimMontage);
}

void ATelekinesisCharacter::Push()
{
	bTelekinesis = false;
	bFaceForward = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	CameraOffsetRightTarget = CameraDefaultOffsetRight;
	CameraOffsetUpTarget = CameraDefaultOffsetUp;
	CameraArmLengthTarget = CameraDefaultArmLength;
	// Call blueprints to handle rotating / zooming in the camera
	Zoom();
	// Determine push line trace and send to the prop
	FVector ImpactPoint = FVector::ZeroVector;
	PushTrace(ImpactPoint);
	CurrTelekineticProp->Push(ImpactPoint);
	// Kill the reference to our held prop
	CurrTelekineticProp = nullptr;
}

void ATelekinesisCharacter::PushTrace(FVector& ImpactPoint)
{
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);
	ActorsToIgnore.Add(TelekineticTarget);

	FHitResult Hit;
	const FVector End = FollowCamera->GetComponentLocation() + (FollowCamera->GetForwardVector() * PushTraceDistance);
	UKismetSystemLibrary::LineTraceSingle(
		GetWorld(),
		FollowCamera->GetComponentLocation(),
		End,
		ETraceTypeQuery::TraceTypeQuery1,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		Hit,
		true
	);

	ImpactPoint	= Hit.ImpactPoint;
}

void ATelekinesisCharacter::AddCameraBoomOffset() const
{
	const FVector Right = UKismetMathLibrary::GetRightVector(GetControlRotation());
	const FVector Up = UKismetMathLibrary::GetUpVector(GetControlRotation());
	const FVector Offset = (Right * CameraDefaultOffsetRight) + (Up * CameraDefaultOffsetUp);
	CameraBoom->SocketOffset = Offset;
}

FVector ATelekinesisCharacter::GetTelekineticPropLocation() const
{
	return PropSceneComponent->GetComponentLocation();
}
