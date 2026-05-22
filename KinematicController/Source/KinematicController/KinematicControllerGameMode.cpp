// Copyright Epic Games, Inc. All Rights Reserved.

#include "KinematicControllerGameMode.h"
#include "KinematicControllerCharacter.h"
#include "UObject/ConstructorHelpers.h"

AKinematicControllerGameMode::AKinematicControllerGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
