// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TelekinesisCharacter.generated.h"

UCLASS(config=Game)
class ATelekinesisCharacter : public ACharacter
{
	GENERATED_BODY()
	
public:
	ATelekinesisCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Input)
	float TurnRateGamepad;
	
	virtual void Tick(float DeltaSeconds) override;
	
	/** Returns CameraBoom sub object **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera sub object **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** Get the location of the PropSceneComponent */
	FVector GetTelekineticPropLocation() const;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void Zoom();
	
	/** Called for forwards/backward input */
	void MoveForward(float Value);
	/** Called for side to side input */
	void MoveRight(float Value);
	/** Called for looking left/right input */
	void TurnRight(float Rate);
	/** Called for forwards/backward input */
	void InputTelekinesis();

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

private:
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	/** Scene Component for held Telekinetic Props */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telekinesis", meta = (AllowPrivateAccess = "true"))
	class USceneComponent* PropSceneComponent;
	
	/** Camera and Camera Boom properties */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess=true))
	float CameraDefaultArmLength = 250.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess=true))
	FVector CameraDefaultLocation = FVector(0.0f, 0.0f, 60.0f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess=true))
	float CameraDefaultOffsetRight = 30.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess=true))
	float CameraDefaultOffsetUp = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess=true))
	float CameraZoomArmLength = 200.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess=true))
	float CameraZoomOffsetRight = 60.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess=true))
	float CameraZoomOffsetUp = 0.0f;
	
    /** Whether or not to force the character model to face directly away from the camera */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Camera", meta=(AllowPrivateAccess=true))
    bool bFaceForward = false;
	
	/** Camera offset variables, targets, mostly for the zoom in/out functionality */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess=true))
	float CameraArmLengthTarget = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess=true))
	float CameraOffsetRightTarget = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess=true))
	float CameraOffsetUpTarget = 0.f;

	/** Whether or not we are currently using Telekinesis */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telekinesis", meta=(AllowPrivateAccess=true))
	bool bTelekinesis = false;
	/** Detection Radius of our Telekinesis */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telekinesis", meta=(AllowPrivateAccess=true))
	float DetectionRadius = 25.f;
	/** Sphere Trace Distance of our Telekinesis  */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telekinesis", meta=(AllowPrivateAccess=true))
	float TelekinesisDistance = 5000.f;
	/** The Actor we are currently using Telekinesis on  */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telekinesis", meta=(AllowPrivateAccess=true))
	class ATelekineticActor* TelekineticTarget = nullptr;
	/** The Actor we are currently using Telekinesis on  */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telekinesis", meta=(AllowPrivateAccess=true))
	class ATelekineticActor* CurrTelekineticProp = nullptr;
	/** The strength of our Push  */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telekinesis", meta=(AllowPrivateAccess=true))
	float PushTraceDistance = 20000.f;
	/** Pull animation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Telekinesis", meta=(AllowPrivateAccess=true))
	class UAnimMontage* PullAnimMontage = nullptr;
	/** Push animation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Telekinesis", meta=(AllowPrivateAccess=true))
	class UAnimMontage* PushAnimMontage = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Telekinesis", meta=(AllowPrivateAccess=true))
	float PushAnimPlayRate = 1.f;
	
	/** Functions for setting up pulling and pushing objects */
	void Push();
	void PushTrace(FVector& ImpactPoint);
	void Pull();

	/** Helper function to add CameraOffsetRight and CameraOffsetUp to camera boom location */
	void AddCameraBoomOffset() const;

};
