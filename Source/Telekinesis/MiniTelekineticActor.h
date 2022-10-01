#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MiniTelekineticActor.generated.h"

UCLASS()
class TELEKINESIS_API AMiniTelekineticActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AMiniTelekineticActor();
	void AttractForce(const FVector& Direction) const;
	TObjectPtr<UStaticMeshComponent> GetMesh() const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Component", meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> TelekineticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attraction", meta=(AllowPrivateAccess = "true"))
	float AttractionForce = 1000.f;
	
};
