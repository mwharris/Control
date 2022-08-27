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

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mesh", meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> TelekineticMesh;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Lift", meta=(AllowPrivateAccess = "true"))
	float LiftDurationSeconds = 0.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Lift", meta=(AllowPrivateAccess = "true"))
	float LiftHeight = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Reach", meta=(AllowPrivateAccess = "true"))
	float ReachSpeedMultiplier = 2.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Reach", meta=(AllowPrivateAccess = "true"))
	float MassMinRange = 50.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Reach", meta=(AllowPrivateAccess = "true"))
	float MassMaxRange = 700.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Reach", meta=(AllowPrivateAccess = "true"))
	float MassMultiplierMinRange = 1.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Reach", meta=(AllowPrivateAccess = "true"))
	float MassMultiplierMaxRange = 5.f;

	class ATelekinesisCharacter* PlayerCharacter = nullptr;
	ETelekinesisStates TelekinesisState = ETelekinesisStates::Default;
	
	// Variables for Lift timer
	FTimerHandle LiftTimerHandle;
	float LiftStartTimeSeconds = 0.f;
	FVector LiftStart = FVector::ZeroVector;
	FVector LiftEnd = FVector::ZeroVector;

	// Variables for Reach timer
	FTimerHandle ReachTimerHandle;

	void StartLift();
	void Lift();
	void StartReach();
	void ReachCharacter() const;
	float GetLiftEndTimeSeconds() const;
	
};
