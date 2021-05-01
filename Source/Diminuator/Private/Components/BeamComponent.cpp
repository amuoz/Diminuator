// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/BeamComponent.h"

#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Gameframework/Character.h"

// Sets default values for this component's properties
UBeamComponent::UBeamComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	BeamRange = 100.0f;
	BeamImpulse = 100.0f;
}

// Called when the game starts
void UBeamComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void UBeamComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UBeamComponent::OnFire(BeamMode Mode)
{

	UWorld* const World = GetWorld();

	/*
	bool LineTraceSingleByChannel
	(
		struct FHitResult& OutHit,
		const FVector & Start,
		const FVector & End,
		ECollisionChannel TraceChannel,
		const FCollisionQueryParams & Params,
		const FCollisionResponseParams & ResponseParam
	) const
	*/

	// We can safely cast this because beam component is Within = ACharacter
	FHitResult OutHit;
	const FRotator SpawnRotation = Cast<ACharacter>(GetOwner())->GetControlRotation();
	// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
	const FVector Start = GetOwner()->GetActorLocation() + SpawnRotation.RotateVector(GunOffset);
	// The end location of line trace is start added with the rotation vector gives forward vector in any rotation
	FVector End = Start + (SpawnRotation.Vector() * BeamRange);

	FCollisionQueryParams params;
	params.AddIgnoredActor(GetOwner());
	// Launch ray!
	bool bHit = World->LineTraceSingleByChannel(OutHit, Start, End, ECollisionChannel::ECC_Visibility, params);

	/*
	void DrawDebugLine
	(
		const UWorld * InWorld,
		FVector const& LineStart,
		FVector const& LineEnd,
		FColor const& Color,
		bool bPersistentLines,
		float LifeTime,
		uint8 DepthPriority,
		float Thickness
	)
	*/
	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.0f, 0, 2.0f);

	if (bHit)
	{
		AActor* hitActor = OutHit.GetActor();
		UPrimitiveComponent* hitComp = OutHit.GetComponent();

		if ((hitActor != nullptr) && (hitActor != GetOwner()) && (hitComp != nullptr))
		{
			FVector ScaleTransformation = (Mode == BeamMode::DIMINUATOR) ? hitComp->GetRelativeScale3D() / 2 : hitComp->GetRelativeScale3D() * 2;
			hitComp->SetRelativeScale3D(ScaleTransformation);
		}

		/*
		// Only add impulse if we hit a physics
		if ((hitActor != nullptr) && (hitActor != GetOwner()) && (hitComp != nullptr) && hitComp->IsSimulatingPhysics())
		{
			hitComp->AddImpulseAtLocation(SpawnRotation.Vector() * BeamImpulse, OutHit.Location);
		}
		*/
	}

}

