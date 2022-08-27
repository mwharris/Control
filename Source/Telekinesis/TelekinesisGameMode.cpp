// Copyright Epic Games, Inc. All Rights Reserved.

#include "TelekinesisGameMode.h"
#include "TelekinesisCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATelekinesisGameMode::ATelekinesisGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
