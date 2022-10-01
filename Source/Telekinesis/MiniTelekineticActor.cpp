#include "MiniTelekineticActor.h"

AMiniTelekineticActor::AMiniTelekineticActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	TelekineticMesh = CreateDefaultSubobject<UStaticMeshComponent>("Telekinetic Mesh");
	SetRootComponent(TelekineticMesh);
}

void AMiniTelekineticActor::BeginPlay()
{
	Super::BeginPlay();
	TelekineticMesh->SetSimulatePhysics(true);
}

void AMiniTelekineticActor::AttractForce(const FVector& Direction) const
{
	TelekineticMesh->AddForce(Direction * AttractionForce, NAME_None, true);
}

TObjectPtr<UStaticMeshComponent> AMiniTelekineticActor::GetMesh() const
{
	return TelekineticMesh;
}
