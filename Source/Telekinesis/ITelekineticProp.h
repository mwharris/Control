#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ITelekineticProp.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UTelekineticProp : public UInterface
{
	GENERATED_BODY()
};

/**
 * Add interface functions to this class. This is the class that will be inherited to implement this interface.
 */
class TELEKINESIS_API ITelekineticProp
{
	GENERATED_BODY()

public:
	virtual void Highlight(bool bHighlight) = 0;
	virtual void Pull(class ATelekinesisCharacter* PlayerCharacter) = 0;
	virtual void Push(FVector Destination) = 0;
	
};
