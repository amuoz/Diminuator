// Tequila Works test
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DiminuatorTypes.h"

#include "BeamComponent.generated.h"

class ADiminuatorCharacter;
class UPrimitiveComponent;
class UPhysicsHandleComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Within = DiminuatorCharacter)
class DIMINUATOR_API UBeamComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	// Sets default values for this component's properties
	UBeamComponent();

	// Called every frame checks beam state
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Fire beam event
	void OnStartFire(BeamMode Mode);

	// Stop beam event
	void OnStopFire(BeamMode Mode);

protected:

	// Called when the game starts
	virtual void BeginPlay() override;

	// Executed when beam is active
	void ShootBeam(float DeltaTime);

	void BeamPhysicsLogic(float DeltaTime);

	void BeamStaticLogic(float HitDistance);

	void TryReleaseObject();

	void BeamEffects(const FVector start, const FVector end);

	/*
	* Check collisions for each cube vertex so we can prevent scaling
	*/
	bool CheckScaleCollisions(UPrimitiveComponent* Component);
	bool CheckVertexCollisions(UPrimitiveComponent* Component, const FVector Direction, const FVector Vertex);
	
	/* 
	* Changes beam state machine depending on user inputs.
	* If both modes are active then grabbing will kick in.
	*/
	void UpdateBeamState(BeamMode NewMode);

	/*
	* Important!
	* Only scale if scaling up and collisions are clear or if scaling down and not min size reached
	*/
	bool CheckScalingConditions(FVector newScale3D);

	FColor GetBeamColor(BeamMode Mode);

	float GetBeamScale(BeamMode Mode);

	bool IsBeamActive();

public:

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

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	FVector GunOffset;

	/* Physics handle linear damping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float GrabLinearDamping;

	/* Time for the grabbed object to disconect if cant follow the beam */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float DisconnectionTime;

	/* Min scaling size */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float MinSize;

protected:

	// Beam state machine
	BeamMode BeamState;

	// Line trace results
	ADiminuatorCharacter* Character;
	UPrimitiveComponent* HitComponent;
	FVector Start;
	float GrabDistance;
	
	// Grabbing component
	UPhysicsHandleComponent* PhysicsHandleComponent;

	// Disconnetion handler
	FTimerHandle StuckTimerHandle;
};
