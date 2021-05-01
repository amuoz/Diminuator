// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DiminuatorHUD.generated.h"

UCLASS()
class ADiminuatorHUD : public AHUD
{
	GENERATED_BODY()

public:
	ADiminuatorHUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};

