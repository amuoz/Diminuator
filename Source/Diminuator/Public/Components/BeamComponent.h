// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DiminuatorTypes.h"

#include "BeamComponent.generated.h"

class UPhysicsHandleComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Within = DiminuatorCharacter)
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

	UPROPERTY(VisibleDefaultsOnly, Category = Physics)
	UPhysicsHandleComponent* PhysicsHandleComponent;

	// Controls beam activation
	bool bDiminuatorActive;

	bool bAugmentatorActive;

	UPrimitiveComponent* HitComponent;
	FVector HitLocation;

public:

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Fire event
	void OnStartFire(BeamMode Mode);

	// Fire event
	void OnStopFire(BeamMode Mode);

protected:

	// Called when the game starts
	virtual void BeginPlay() override;

	void ShootBeam(BeamMode Mode);

	void StopBeam();

	void BeamTransformation(BeamMode Mode, float DeltaTime);

	FColor GetBeamColor(BeamMode Mode);

	float GetBeamScale(BeamMode Mode);

	bool IsBeamActive();
};
