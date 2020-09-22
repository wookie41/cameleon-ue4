// Copyright Epic Games, Inc. All Rights Reserved.

#include "CameleonGameGameMode.h"
#include "CameleonGameHUD.h"
#include "CameleonGameCharacter.h"
#include "UObject/ConstructorHelpers.h"

ACameleonGameGameMode::ACameleonGameGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = ACameleonGameHUD::StaticClass();
}
