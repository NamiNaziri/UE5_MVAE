// Copyright Epic Games, Inc. All Rights Reserved.

#include "MVAEGameMode.h"
#include "MVAECharacter.h"
#include "UObject/ConstructorHelpers.h"

AMVAEGameMode::AMVAEGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
