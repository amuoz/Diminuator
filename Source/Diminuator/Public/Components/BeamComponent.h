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

	/* Interpolation physics handle speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float BeamThickness;

	/* Interpolation physics handle speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UMaterial* GlassMaterial;

	/* Physics handle linear damping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float GrabLinearDamping;

protected:

	UPhysicsHandleComponent* PhysicsHandleComponent;

	// Controls beam activation
	bool bDiminuatorActive;

	bool bAugmentatorActive;

	float GrabDistance;

	FTimerHandle StuckTimerHandle;

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

	void ShootBeam(BeamMode Mode, float DeltaTime);

	FColor GetBeamColor(BeamMode Mode);

	float GetBeamScale(BeamMode Mode);

	bool IsBeamActive();

	bool CheckScaleCollisions(UPrimitiveComponent* Component);

	bool CheckVertexCollisions(UPrimitiveComponent* Component, FVector Direction, FVector Vertex);

	bool IsGlassMaterial(const UMaterial* Material);

	void OnStuck();
};
