// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/BeamComponent.h"

#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "DiminuatorCharacter.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Components/MeshComponent.h"

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
	BeamThickness = 3.0f;

	// Physics handle 
	PhysicsHandleComponent = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandleComponent"));
	PhysicsHandleComponent->SetLinearDamping(40.0f);
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
		ShootBeam(BeamMode::BOTH, DeltaTime);	// Special mode?
	}
	else if (bDiminuatorActive)
	{
		ShootBeam(BeamMode::DIMINUATOR, DeltaTime);
	}
	else if (bAugmentatorActive)
	{
		ShootBeam(BeamMode::AUGMENTATOR, DeltaTime);
	}

}

void UBeamComponent::OnStartFire(BeamMode Mode)
{
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

	if (!IsBeamActive())
	{
		if (PhysicsHandleComponent->IsActive())
		{
			PhysicsHandleComponent->SetActive(false);
			PhysicsHandleComponent->ReleaseComponent();
		}
	}

}

void UBeamComponent::ShootBeam(BeamMode Mode, float DeltaTime)
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
			//const FName TraceTag("MyTraceTag");
			//GetWorld()->DebugDrawTraceTag = TraceTag;
			//params.TraceTag = TraceTag;

			// if something is grabbed check if there is collision between the beam and the object ideal location
			if (Mode == BeamMode::BOTH && PhysicsHandleComponent->IsActive() && PhysicsHandleComponent->GetGrabbedComponent())
			{
				end = start + (spawnRotation.Vector() * GrabDistance);
				params.AddIgnoredActor(PhysicsHandleComponent->GetGrabbedComponent()->GetOwner());
			}

			// Launch beam raycast searching for object
			bool bHit = world->LineTraceSingleByChannel(outHit, start, end, ECollisionChannel::ECC_Visibility, params);
			if (bHit)
			{
				// Beam effect hit
				DrawDebugLine(world, start, outHit.Location, GetBeamColor(Mode), false, -1.0f, 0, BeamThickness);

				AActor* hitActor = outHit.GetActor();
				UPrimitiveComponent* hitComp = outHit.GetComponent();
				
				// lets make sure the object is valid
				if ((hitActor != nullptr) && (hitActor != GetOwner()) && (hitComp != nullptr))
				{
					// physics actor interaction
					if (hitComp->IsSimulatingPhysics())
					{
						if (Mode == BeamMode::BOTH)
						{
							//FVector grabEnd = start + (spawnRotation.Vector() * (start - outHit.Location).Size());
							if (hitComp != PhysicsHandleComponent->GetGrabbedComponent())
							{
								// The end location of line trace is start added with the rotation vector gives forward vector in any rotation
								PhysicsHandleComponent->SetActive(true);
								PhysicsHandleComponent->GrabComponentAtLocationWithRotation(hitComp, outHit.BoneName, hitComp->GetComponentLocation(), hitComp->GetComponentRotation());
								//GrabDistance = (start - outHit.Location).Size();
								GrabDistance = (start - hitComp->GetComponentLocation()).Size();
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
							hitComp->SetSimulatePhysics(false);
							FVector scaleTransformation = hitComp->GetRelativeScale3D() + GetBeamScale(Mode) * DeltaTime;
							if (scaleTransformation.Size() > 0.2f)
							{
								if (scaleTransformation.Size() > hitComp->GetRelativeScale3D().Size())
								{
									if (!CheckScaleCollisions(hitComp))
									{
										hitComp->SetRelativeScale3D(scaleTransformation);
									}
								}
								else
								{
									hitComp->SetRelativeScale3D(scaleTransformation);
								}
							}
							// turn on physics to recalculate collisions in physics step
							hitComp->SetSimulatePhysics(true);
						}
					}
					// static actor hit
					else
					{
						// check if beam is far from the grabbed object
						if (PhysicsHandleComponent->IsActive() && PhysicsHandleComponent->GetGrabbedComponent() != nullptr && !IsGlassMaterial(hitComp->GetMaterial(0)->GetMaterial()))
						{
							PhysicsHandleComponent->SetActive(false);
							PhysicsHandleComponent->ReleaseComponent();

							/*
							// Grab point
							FVector grabEnd = (spawnRotation.Vector() * GrabDistance);
							FVector grabComp = PhysicsHandleComponent->GetGrabbedComponent()->GetComponentLocation() - start;
							float distance = (grabComp - grabEnd).Size() - PhysicsHandleComponent->GetGrabbedComponent()->Bounds.BoxExtent.X/2;
							UE_LOG(LogTemp, Warning, TEXT("Distance: %f"), distance);
							if (distance > 200.0f)
							{
								// Beam disconnection
								PhysicsHandleComponent->SetActive(false);
								PhysicsHandleComponent->ReleaseComponent();
							}
							*/
						}
					}
				}
			}
			else
			{
				// Beam effect no collision
				DrawDebugLine(world, start, end, GetBeamColor(Mode), false, -1.0f, 0, BeamThickness);
			}

			// check if grabbed something
			if (PhysicsHandleComponent->IsActive() && PhysicsHandleComponent->GetGrabbedComponent())
			{
				FVector grabEnd = start + (spawnRotation.Vector() * GrabDistance);
				PhysicsHandleComponent->SetTargetLocationAndRotation(grabEnd, PhysicsHandleComponent->GetGrabbedComponent()->GetComponentRotation());
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

bool UBeamComponent::CheckScaleCollisions(UPrimitiveComponent* component)
{
	if (component != nullptr)
	{
		FHitResult outHit;
		FVector start = component->GetComponentLocation();
		float length = component->Bounds.SphereRadius/2;
		float size = length/4;
		FCollisionQueryParams params;
		params.AddIgnoredActor(component->GetOwner());
		const FName TraceTag("MyTraceTag");
		GetWorld()->DebugDrawTraceTag = TraceTag;
		params.TraceTag = TraceTag;

		//bool bUp = GetWorld()->LineTraceSingleByChannel(outHit, start, (start + FVector::UpVector * length), ECollisionChannel::ECC_Visibility, params);
		bool bUp1 = CheckVertexCollisions(component, FVector::UpVector, FVector::UpVector + FVector::LeftVector + FVector::ForwardVector);
		bool bUp2 = CheckVertexCollisions(component, FVector::UpVector, FVector::UpVector + FVector::LeftVector + FVector::BackwardVector);
		bool bUp3 = CheckVertexCollisions(component, FVector::UpVector, FVector::UpVector + FVector::RightVector + FVector::ForwardVector);
		bool bUp4 = CheckVertexCollisions(component, FVector::UpVector, FVector::UpVector + FVector::RightVector + FVector::BackwardVector);
		bool bUp = bUp1 || bUp2 || bUp3 || bUp4;

		//bool bDown = GetWorld()->LineTraceSingleByChannel(outHit, start, (start + FVector::DownVector * length), ECollisionChannel::ECC_Visibility, params);
		bool bDown1 = CheckVertexCollisions(component, FVector::DownVector, FVector::DownVector + FVector::LeftVector + FVector::ForwardVector);
		bool bDown2 = CheckVertexCollisions(component, FVector::DownVector, FVector::DownVector + FVector::LeftVector + FVector::BackwardVector);
		bool bDown3 = CheckVertexCollisions(component, FVector::DownVector, FVector::DownVector + FVector::RightVector + FVector::ForwardVector);
		bool bDown4 = CheckVertexCollisions(component, FVector::DownVector, FVector::DownVector + FVector::RightVector + FVector::BackwardVector);
		bool bDown = bDown1 || bDown2 || bDown3 || bDown4;

		//bool bLeft = GetWorld()->LineTraceSingleByChannel(outHit, start, (start + FVector::LeftVector * length), ECollisionChannel::ECC_Visibility, params);
		bool bLeft1 = CheckVertexCollisions(component, FVector::LeftVector, FVector::LeftVector + FVector::UpVector + FVector::ForwardVector);
		bool bLeft2 = CheckVertexCollisions(component, FVector::LeftVector, FVector::LeftVector + FVector::UpVector + FVector::BackwardVector);
		bool bLeft3 = CheckVertexCollisions(component, FVector::LeftVector, FVector::LeftVector + FVector::DownVector + FVector::ForwardVector);
		bool bLeft4 = CheckVertexCollisions(component, FVector::LeftVector, FVector::LeftVector + FVector::DownVector + FVector::BackwardVector);
		bool bLeft = bLeft1 || bLeft2 || bLeft3 || bLeft4;

		//bool bRight = GetWorld()->LineTraceSingleByChannel(outHit, start, (start + FVector::RightVector * length), ECollisionChannel::ECC_Visibility, params);
		bool bRight1 = CheckVertexCollisions(component, FVector::RightVector, FVector::RightVector + FVector::UpVector + FVector::ForwardVector);
		bool bRight2 = CheckVertexCollisions(component, FVector::RightVector, FVector::RightVector + FVector::UpVector + FVector::BackwardVector);
		bool bRight3 = CheckVertexCollisions(component, FVector::RightVector, FVector::RightVector + FVector::DownVector + FVector::ForwardVector);
		bool bRight4 = CheckVertexCollisions(component, FVector::RightVector, FVector::RightVector + FVector::DownVector + FVector::BackwardVector);
		bool bRight = bRight1 || bRight2 || bRight3 || bRight4;

		//bool bForward = GetWorld()->LineTraceSingleByChannel(outHit, start, (start + FVector::ForwardVector * length), ECollisionChannel::ECC_Visibility, params);
		bool bForward1 = CheckVertexCollisions(component, FVector::ForwardVector, FVector::ForwardVector + FVector::UpVector + FVector::LeftVector);
		bool bForward2 = CheckVertexCollisions(component, FVector::ForwardVector, FVector::ForwardVector + FVector::UpVector + FVector::RightVector);
		bool bForward3 = CheckVertexCollisions(component, FVector::ForwardVector, FVector::ForwardVector + FVector::DownVector + FVector::LeftVector);
		bool bForward4 = CheckVertexCollisions(component, FVector::ForwardVector, FVector::ForwardVector + FVector::DownVector + FVector::RightVector);
		bool bForward = bForward1 || bForward2 || bForward3 || bForward4;

		//bool bBackward = GetWorld()->LineTraceSingleByChannel(outHit, start, (start + FVector::BackwardVector * length), ECollisionChannel::ECC_Visibility, params);
		bool bBackward1 = CheckVertexCollisions(component, FVector::BackwardVector, FVector::BackwardVector + FVector::UpVector + FVector::LeftVector);
		bool bBackward2 = CheckVertexCollisions(component, FVector::BackwardVector, FVector::BackwardVector + FVector::UpVector + FVector::RightVector);
		bool bBackward3 = CheckVertexCollisions(component, FVector::BackwardVector, FVector::BackwardVector + FVector::DownVector + FVector::LeftVector);
		bool bBackward4 = CheckVertexCollisions(component, FVector::BackwardVector, FVector::BackwardVector + FVector::DownVector + FVector::RightVector);
		bool bBackward = bBackward1 || bBackward2 || bBackward3 || bBackward4;

		return (bUp && bDown) || (bLeft && bRight) || (bForward && bBackward);
	}

	return false;
}

bool UBeamComponent::IsGlassMaterial(const UMaterial* Material)
{
	return Material->GetName().Equals(GlassMaterial->GetName());
}

bool UBeamComponent::CheckVertexCollisions(UPrimitiveComponent* component, FVector direction, FVector vertex)
{
	FHitResult outHit;
	float length = component->Bounds.SphereRadius / 2;
	float size = length / 4;
	FCollisionQueryParams params;
	params.AddIgnoredActor(component->GetOwner());
	//const FName TraceTag("MyTraceTag");
	//GetWorld()->DebugDrawTraceTag = TraceTag;
	//params.TraceTag = TraceTag;

	FVector start = component->GetComponentLocation() + component->GetComponentRotation().RotateVector(vertex * length);
	FVector end = start + component->GetComponentRotation().RotateVector(direction) * size;
	return GetWorld()->LineTraceSingleByChannel(outHit, start, end, ECollisionChannel::ECC_WorldStatic, params);
}
