// Copyright Epic Games, Inc. All Rights Reserved.

#include "DiminuatorGameMode.h"
#include "DiminuatorHUD.h"
#include "DiminuatorCharacter.h"
#include "UObject/ConstructorHelpers.h"

ADiminuatorGameMode::ADiminuatorGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = ADiminuatorHUD::StaticClass();
}
