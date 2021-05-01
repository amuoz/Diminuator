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
	PrimaryComponentTick.bCanEverTick = true;

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	BeamRange = 100.0f;
	BeamImpulse = 100.0f;
	BeamScaleSpeed = 5.0f;
	bDiminuatorActive = false;
	bAugmentatorActive = false;
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

	if (bDiminuatorActive && bAugmentatorActive)
	{
		// if both beams are active, merge colors and no transformation is applied
		ShootBeam(BeamMode::BOTH, DeltaTime);	// Special mode?
	}
	else if (bDiminuatorActive)
	{
		ShootBeam(BeamMode::AUGMENTATOR, DeltaTime);
	}
	else if (bAugmentatorActive)
	{
		ShootBeam(BeamMode::DIMINUATOR, DeltaTime);
	}

}

void UBeamComponent::OnFire(BeamMode Mode, bool Active)
{
	if (Mode == BeamMode::DIMINUATOR)
	{
		bDiminuatorActive = Active;
	}

	if (Mode == BeamMode::AUGMENTATOR)
	{
		bAugmentatorActive = Active;
	}
}

void UBeamComponent::ShootBeam(BeamMode Mode, float DeltaTime)
{
	UWorld* const world = GetWorld();
	if (world != nullptr)
	{
		ACharacter* character = Cast<ACharacter>(GetOwner());
		if (character != nullptr)
		{
			FHitResult outHit;
			// We can safely cast this because beam component is Within = ACharacter
			const FRotator spawnRotation = character->GetControlRotation();
			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			const FVector start = GetOwner()->GetActorLocation() + spawnRotation.RotateVector(GunOffset);
			// The end location of line trace is start added with the rotation vector gives forward vector in any rotation
			FVector end = start + (spawnRotation.Vector() * BeamRange);
			FCollisionQueryParams params;
			params.AddIgnoredActor(GetOwner());

			// Launch beam!
			DrawDebugLine(world, start, end, GetBeamColor(Mode), false, DeltaTime, 0, 2.0f);

			// if both beams are active skip transformation
			if (Mode != BeamMode::BOTH)
			{
				bool bHit = world->LineTraceSingleByChannel(outHit, start, end, ECollisionChannel::ECC_Visibility, params);
				// Scale transformation logic
				if (bHit)
				{
					AActor* hitActor = outHit.GetActor();
					UPrimitiveComponent* hitComp = outHit.GetComponent();

					if ((hitActor != nullptr) && (hitActor != GetOwner()) && (hitComp != nullptr))
					{
						FVector scaleTransformation = hitComp->GetRelativeScale3D() + GetBeamScale(Mode) * DeltaTime;
						hitComp->SetRelativeScale3D(scaleTransformation);
					}
				}
			}

		}
	}
}

FColor UBeamComponent::GetBeamColor(BeamMode Mode)
{
	FColor beamColor = FColor::White;
	switch (Mode)
	{
	case BeamMode::DIMINUATOR:
		beamColor = FColor::Blue;
		break;

	case BeamMode::AUGMENTATOR:
		beamColor = FColor::Red;
		break;
	
	case BeamMode::BOTH:
		beamColor = FColor::Magenta;
		break;
	}

	return beamColor;
}

float UBeamComponent::GetBeamScale(BeamMode Mode)
{
	switch (Mode)
	{
	case BeamMode::DIMINUATOR:
		return -BeamScaleSpeed;
		break;

	case BeamMode::AUGMENTATOR:
		return BeamScaleSpeed;
		break;
	}

	return 0.0f;
}
