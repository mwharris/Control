#pragma once

#include "CoreMinimal.h"
#include "Engine/UserDefinedEnum.h"
#include "ETelekinesisStates.generated.h"

UENUM(BlueprintType)
enum class ETelekinesisStates : uint8 {
	Default UMETA(DisplayName="Default"),
	Pulled  UMETA(DisplayName="Pulled"),
	Pushed	UMETA(DisplayName="Pushed"),
};
