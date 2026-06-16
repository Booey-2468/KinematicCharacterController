// Copyright Epic Games, Inc. All Rights Reserved.

#include "KinematicControllerGameMode.h"
#include "UObject/ConstructorHelpers.h"

AKinematicControllerGameMode::AKinematicControllerGameMode()
{
	// set default pawn class to My Kinematic Character Controller found by a file path from game source
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/KinematicController/BP_KinematicCharacterController"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
