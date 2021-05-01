// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DiminuatorTypes.h"

#include "BeamComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Within = Character)
class DIMINUATOR_API UBeamComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBeamComponent();

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	FVector GunOffset;

	/* Beam line trace range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float BeamRange;

	/* Beam test impulse */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float BeamImpulse;

	/* Beam scaling speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float BeamScaleSpeed;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void ShootBeam(BeamMode Mode, float DeltaTime);

	FColor GetBeamColor(BeamMode Mode);

	float GetBeamScale(BeamMode Mode);

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Fire event
	void OnFire(BeamMode Mode, bool Active);

private:
	// Controls beam activation
	bool bDiminuatorActive;
	bool bAugmentatorActive;
};
