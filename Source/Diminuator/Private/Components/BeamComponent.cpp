// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/BeamComponent.h"

#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "DiminuatorCharacter.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"

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

	// Physics handle 
	PhysicsHandleComponent = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandleComponent"));
	PhysicsHandleComponent->SetActive(false);
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
		BeamTransformation(BeamMode::BOTH, DeltaTime);	// Special mode?
	}
	else if (bDiminuatorActive)
	{
		BeamTransformation(BeamMode::DIMINUATOR, DeltaTime);
	}
	else if (bAugmentatorActive)
	{
		BeamTransformation(BeamMode::AUGMENTATOR, DeltaTime);
	}

}

void UBeamComponent::OnStartFire(BeamMode Mode)
{
	// If beam is active we allready have object
	if(!IsBeamActive())
	{
		ShootBeam(Mode);
	}
	
	if (Mode == BeamMode::DIMINUATOR)
	{
		bDiminuatorActive = true;
	}
	else if (Mode == BeamMode::AUGMENTATOR)
	{
		bAugmentatorActive = true;
	}

}

void UBeamComponent::OnStopFire(BeamMode Mode)
{
	if (Mode == BeamMode::DIMINUATOR)
	{
		bDiminuatorActive = false;
	}
	else if (Mode == BeamMode::AUGMENTATOR)
	{
		bAugmentatorActive = false;
	}

	// If beam is active we allready have object
	if (!IsBeamActive())
	{
		StopBeam();
	}
}

void UBeamComponent::ShootBeam(BeamMode Mode)
{
	UWorld* const world = GetWorld();
	if (world != nullptr)
	{
		// We can safely cast this because beam component is Within = DiminuatorCharacter
		ADiminuatorCharacter* character = Cast<ADiminuatorCharacter>(GetOwner());
		if (character != nullptr)
		{
			FHitResult outHit;
			const FRotator spawnRotation = character->GetControlRotation();
			const FVector start = (character->GetFP_MuzzleLocation() != nullptr) ? character->GetFP_MuzzleLocation()->GetComponentLocation() : GetOwner()->GetActorLocation() + spawnRotation.RotateVector(GunOffset);
			// The end location of line trace is start added with the rotation vector gives forward vector in any rotation
			FVector end = start + (spawnRotation.Vector() * BeamRange);
			FCollisionQueryParams params;
			params.AddIgnoredActor(GetOwner());

			// Beam effect
			DrawDebugLine(world, start, end, GetBeamColor(Mode), false, -1.0f, 0, 2.0f);

			// Launch beam raycast searching for object
			bool bHit = world->LineTraceSingleByChannel(outHit, start, end, ECollisionChannel::ECC_Visibility, params);
			if (bHit)
			{
				AActor* hitActor = outHit.GetActor();
				UPrimitiveComponent* hitComp = outHit.GetComponent();
				// lets make sure the object is valid
				if ((hitActor != nullptr) && (hitActor != GetOwner()) && (hitComp != nullptr) && hitComp->IsSimulatingPhysics())
				{
					// Grab physics object
					HitComponent = hitComp;
					HitLocation = outHit.Location;
				}
			}
		}
	}
}

void UBeamComponent::StopBeam()
{
	if (PhysicsHandleComponent->IsActive())
	{
		PhysicsHandleComponent->SetActive(false);
		PhysicsHandleComponent->ReleaseComponent();
	}

	if (HitComponent)
	{
		HitComponent->SetSimulatePhysics(true);
		HitComponent = nullptr;
		HitLocation = FVector();
	}
}

void UBeamComponent::BeamTransformation(BeamMode Mode, float DeltaTime)
{
	UWorld* const world = GetWorld();
	if (world != nullptr)
	{
		// We can safely cast this because beam component is Within = DiminuatorCharacter
		ADiminuatorCharacter* character = Cast<ADiminuatorCharacter>(GetOwner());
		if (character != nullptr)
		{
			//UPrimitiveComponent* hitComp = PhysicsHandleComponent->GetGrabbedComponent();
			if (HitComponent != nullptr)
			{
				// We can safely cast this because beam component is Within = ACharacter
				const FRotator spawnRotation = character->GetControlRotation();
				// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
				const FVector start = (character->GetFP_MuzzleLocation() != nullptr) ? character->GetFP_MuzzleLocation()->GetComponentLocation() : GetOwner()->GetActorLocation() + spawnRotation.RotateVector(GunOffset);

				if (Mode == BeamMode::BOTH)
				{
					HitComponent->SetSimulatePhysics(true);

					// The end location of line trace is start added with the rotation vector gives forward vector in any rotation
					FVector end = start + (spawnRotation.Vector() * (start - HitLocation).Size());
					// Beam effect
					DrawDebugLine(world, start, end, GetBeamColor(Mode), false, -1.0f, 0, 2.0f);

					if (!PhysicsHandleComponent->IsActive())
					{
						PhysicsHandleComponent->SetActive(true);
						PhysicsHandleComponent->GrabComponentAtLocation(HitComponent, NAME_None, end);
					}
					else
					{
						PhysicsHandleComponent->SetTargetLocation(end);
					}

				}
				else
				{
					if (PhysicsHandleComponent->IsActive())
					{
						PhysicsHandleComponent->SetActive(false);
						PhysicsHandleComponent->ReleaseComponent();
					}

					// turn off physics for the object to freeze in the air
					HitComponent->SetSimulatePhysics(false);
					FVector scaleTransformation = HitComponent->GetRelativeScale3D() + GetBeamScale(Mode) * DeltaTime;
					HitComponent->SetRelativeScale3D(scaleTransformation);
					FVector hitTransformation = HitLocation + GetBeamScale(Mode) * DeltaTime;
					HitLocation = hitTransformation;
					// turn on physics to recalculate collisions in physics step
					HitComponent->SetSimulatePhysics(true);

					// Beam effect
					DrawDebugLine(world, start, hitTransformation, GetBeamColor(Mode), false, -1.0f, 0, 2.0f);
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

bool UBeamComponent::IsBeamActive()
{
	return bDiminuatorActive || bAugmentatorActive;
}
