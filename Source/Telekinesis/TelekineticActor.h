#pragma once

#include "CoreMinimal.h"
#include "ETelekinesisStates.h"
#include "ITelekineticProp.h"
#include "GameFramework/Actor.h"
#include "TelekineticActor.generated.h"

UCLASS()
class TELEKINESIS_API ATelekineticActor : public AActor, public ITelekineticProp
{
	GENERATED_BODY()
	
public:	
	ATelekineticActor();
	// virtual void Tick(float DeltaTime) override;
	
	// ITelekineticProp interface
	virtual void Highlight(bool bHighlight) override;
	virtual void Pull(ATelekinesisCharacter* InPlayerCharacter) override;
	virtual void Push(FVector Destination) override;
	// End of ITelekineticProp interface
	
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnHitCallback(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	UFUNCTION(BlueprintImplementableEvent)
	void ActivateParticleSystem();
	UFUNCTION(BlueprintImplementableEvent)
	void DeactivateParticleSystem();
	UFUNCTION(BlueprintImplementableEvent)
	void FeedLocationToParticleSystem();
	UFUNCTION(BlueprintImplementableEvent)
	void SpawnSparks(const FVector& Impulse);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Component", meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> TelekineticMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Component", meta = (AllowPrivateAccess = "true"))
	class USphereComponent* AttractionField;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Component", meta = (AllowPrivateAccess = "true"))
	class UAudioComponent* AudioComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Lift", meta=(AllowPrivateAccess = "true"))
	float LiftDurationSeconds = 0.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Lift", meta=(AllowPrivateAccess = "true"))
	float LiftHeight = 150.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Lift", meta=(AllowPrivateAccess = "true"))
	FVector LiftAngularImpulseDirection = FVector(1.f, 1.f, 1.f);
	/** Minimum angular impulse applied to lifted objects so they aren't static */ 
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Lift", meta=(AllowPrivateAccess = "true"))
	float LiftAngularImpulseMinStrength = 400.f;
	/** Maximum angular impulse applied to lifted objects so they aren't static */ 
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Lift", meta=(AllowPrivateAccess = "true"))
	float LiftAngularImpulseMaxStrength = 800.f;
	/** The percentage of our Lift phase at which we'll start the Reach phase for a smooth transition */ 
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Lift", meta=(AllowPrivateAccess = "true"))
	float LiftReachTransitionPercent = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Reach", meta=(AllowPrivateAccess = "true"))
	float PullSpeedMultiplier = 1.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Reach", meta=(AllowPrivateAccess = "true"))
	float PushSpeedMultiplier = 800.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Reach", meta=(AllowPrivateAccess = "true"))
	float MassMinRange = 50.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Reach", meta=(AllowPrivateAccess = "true"))
	float MassMaxRange = 700.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Reach", meta=(AllowPrivateAccess = "true"))
	float MassMultiplierMinRange = 1.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Reach", meta=(AllowPrivateAccess = "true"))
	float MassMultiplierMaxRange = 5.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Reach", meta=(AllowPrivateAccess = "true"))
	float CollisionBounciness = 2.f;
	
	/** Min frame time for object to randomly Jitter */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Jitter", meta=(AllowPrivateAccess = "true"))
	float JitterFrameTimeRangeMin = 10.f;
	/** Max frame time for object to randomly Jitter */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Jitter", meta=(AllowPrivateAccess = "true"))
	float JitterFrameTimeRangeMax = 30.f;
	/** Minimum strength of a Jitter */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Jitter", meta=(AllowPrivateAccess = "true"))
	float JitterStrengthMinMultiplier = 100.f;
	/** Maximum strength of a Jitter */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Jitter", meta=(AllowPrivateAccess = "true"))
	float JitterStrengthMaxMultiplier = 300.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="VFX", meta=(AllowPrivateAccess = "true"))
	float NiagaraSpawnRate = 2000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Sounds", meta=(AllowPrivateAccess = "true"))
	class USoundBase* PushSound = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Sounds", meta=(AllowPrivateAccess = "true"))
	class USoundBase* LiftSound = nullptr;

	// Variables for Lift
	FTimerHandle LiftTimerHandle;
	float LiftStartTimeSeconds = 0.f;
	FVector LiftStart = FVector::ZeroVector;
	FVector LiftEnd = FVector::ZeroVector;

	// Variables for Reach
	FTimerHandle ReachTimerHandle;
	FVector ReachTarget = FVector::ZeroVector;

	// Variables for Jitter
	int32 JitterFrameTime = 0;
	int32 JitterCounter = 0;

	// Variables for attracting AMiniTelekineticActors
	TArray<class AMiniTelekineticActor*> AttractedMiniProps;
	
	// Other variables
	class ATelekinesisCharacter* PlayerCharacter = nullptr;
	ETelekinesisStates TelekinesisState = ETelekinesisStates::Default;
	FVector PushDestination = FVector::ZeroVector;
	FVector PushDirection = FVector::ZeroVector;

	// Lift phase
	void StartLift();
	void Lift();

	// Reach phase
	void StartReach(bool bReachCharacter);
	void ReachCharacter();
	void ReachPoint();
	void ReachLocation(const FVector& Target, float ReachSpeedMultiplier, bool bConstantSpeed);
	void ClearReachTimer();

	// Mini Prop Attraction
	UFUNCTION()
	void OnBeginOverlap(class UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnEndOverlap(class UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	void DetectMiniProps();
	void AttractMiniProps();
	void AddMiniProp(class AMiniTelekineticActor* MiniProp);
	void RemoveMiniProp(class AMiniTelekineticActor* MiniProp);
	
	// Other functions
	void Jitter();
	float GetLiftEndTimeSeconds() const;
	
};
