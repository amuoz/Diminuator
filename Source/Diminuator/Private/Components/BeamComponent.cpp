// Tequila Works test
#include "Components/BeamComponent.h"

#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "DiminuatorCharacter.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Components/MeshComponent.h"
#include "TimerManager.h"

// Sets default values for this component's properties
UBeamComponent::UBeamComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// Default offset from the character location
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	BeamState = BeamMode::OFF;
	BeamRange = 100.0f;
	BeamImpulse = 100.0f;
	BeamScaleSpeed = 5.0f;
	BeamThickness = 3.0f;
	GrabLinearDamping = 60.0f;
	DisconnectionTime = 1.0f;
	MinSize = 0.3f;

	// Physics handle 
	PhysicsHandleComponent = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandleComponent"));
	PhysicsHandleComponent->bInterpolateTarget = false;				// fast follow
	PhysicsHandleComponent->SetLinearDamping(GrabLinearDamping);	// linear damping adjustment
	PhysicsHandleComponent->SetActive(false);						// only active when needed
}

// Called when the game starts
void UBeamComponent::BeginPlay()
{
	Super::BeginPlay();

	// Safe cast because beam component is Within = DiminuatorCharacter
	Character = Cast<ADiminuatorCharacter>(GetOwner());
}

// Called every frame
void UBeamComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Beam is firing
	if (IsBeamActive())
	{
		ShootBeam(DeltaTime);
	}
}

void UBeamComponent::OnStartFire(BeamMode Mode)
{
	UpdateBeamState(Mode);
}

void UBeamComponent::OnStopFire(BeamMode Mode)
{
	UpdateBeamState(Mode);

	// If beam is off release any grabbed object
	if (!IsBeamActive())
	{
		TryReleaseObject();
	}
}

void UBeamComponent::ShootBeam(float DeltaTime)
{
	UWorld* const world = GetWorld();
	if (world != nullptr)
	{
		if (Character != nullptr)
		{
			FHitResult outHit;
			const FRotator spawnRotation = Character->GetControlRotation();
			// The start location of the beam, important for all the logic
			Start = (Character->GetFP_MuzzleLocation() != nullptr) ?
				Character->GetFP_MuzzleLocation()->GetComponentLocation() :
				GetOwner()->GetActorLocation() + spawnRotation.RotateVector(GunOffset);
			// The end location of line trace is start added with the rotation vector gives forward vector in any rotation
			FVector end = Start + (spawnRotation.Vector() * BeamRange);
			FCollisionQueryParams params;
			params.AddIgnoredActor(GetOwner());
			
			// Launch beam searching for objects
			bool bHit = world->LineTraceSingleByChannel(outHit, Start, end, ECollisionChannel::ECC_Visibility, params);
			if (bHit)
			{
				// Lets make sure hit component is valid
				AActor* hitActor = outHit.GetActor();
				HitComponent = outHit.GetComponent();
				if ((hitActor != nullptr) && (hitActor != GetOwner()) && (HitComponent != nullptr))
				{
					// A) Physics actor transformations
					if (HitComponent->IsSimulatingPhysics())
					{
						BeamPhysicsLogic(DeltaTime);
					}
					// B) Static actor non interactuable
					else
					{
						// Dropping logic only if physics handle is active
						if (PhysicsHandleComponent->IsActive())
						{
							float hitDistance = (Start - outHit.Location).Size();
							BeamStaticLogic(hitDistance);
						}
					}
				}
			}

			// Track grabbed object
			if (PhysicsHandleComponent->IsActive() && PhysicsHandleComponent->GetGrabbedComponent() != nullptr)
			{
				FVector grabEnd = Start + (spawnRotation.Vector() * GrabDistance);
				PhysicsHandleComponent->SetTargetLocationAndRotation(grabEnd, PhysicsHandleComponent->GetGrabbedComponent()->GetComponentRotation());
				BeamEffects(Start, grabEnd);	// Play beam effect
			}
			else
			{
				BeamEffects(Start, (bHit ? outHit.Location : end));	// Play beam effect
			}

		}
	}
}


void UBeamComponent::BeamPhysicsLogic(float DeltaTime)
{
	// Grabbing Mode
	if (BeamState == BeamMode::GRAB)
	{
		// We found an object, reset disconnection timer
		GetWorld()->GetTimerManager().ClearTimer(StuckTimerHandle);
		// If its a new object update physics handler
		if (HitComponent != PhysicsHandleComponent->GetGrabbedComponent())
		{
			PhysicsHandleComponent->SetActive(true);
			PhysicsHandleComponent->GrabComponentAtLocationWithRotation(HitComponent, NAME_None, HitComponent->GetComponentLocation(), HitComponent->GetComponentRotation());
			// Grab distance needed for knowing if hits are behind or in front of the component
			GrabDistance = (Start - HitComponent->GetComponentLocation()).Size();
		}
	}
	// Scaling Mode
	else
	{
		// Release object if physics handler active
		TryReleaseObject();
		FVector newScale3D = HitComponent->GetRelativeScale3D() + GetBeamScale(BeamState) * DeltaTime;
		// If scaling up and collisions are clear or if scaling down and not min size reached
		if(CheckScalingConditions(newScale3D))
		{
			// turn off physics before scale for cool effect of freeze in the air
			HitComponent->SetSimulatePhysics(false);
			HitComponent->SetRelativeScale3D(newScale3D);
			// turn on physics to recalculate collisions in physics step
			HitComponent->SetSimulatePhysics(true);
		}
	}
}

void UBeamComponent::BeamStaticLogic(float HitDistance)
{
	// If we hit something in between then release the cube inmediately
	if (HitDistance <= GrabDistance)
	{
		TryReleaseObject();
	}
	// If we hit something in the back 1.the cube could be stucked or 2.we are moving too fast
	else
	{
		// Launch disconnection timer, if we hit the cube again the timer will be cleared
		if (!StuckTimerHandle.IsValid())
		{
			GetWorld()->GetTimerManager().SetTimer(StuckTimerHandle, this, &UBeamComponent::TryReleaseObject, DisconnectionTime, false);
		}
	}
}

void UBeamComponent::TryReleaseObject()
{
	if (PhysicsHandleComponent->IsActive())
	{
		PhysicsHandleComponent->ReleaseComponent();
		PhysicsHandleComponent->SetActive(false);	
	}
}

void UBeamComponent::BeamEffects(const FVector start, const FVector end)
{
	// Trail VFX and sound effects should be here
	DrawDebugLine(GetWorld(), start, end, GetBeamColor(BeamState), false, -1.0f, 0, BeamThickness);	// development only
}

void UBeamComponent::UpdateBeamState(BeamMode NewMode)
{
	if (BeamState == BeamMode::OFF)
	{
		BeamState = NewMode;
	}
	else if (BeamState == BeamMode::GRAB)
	{
		if (NewMode == BeamMode::DIMINUATOR)
		{
			BeamState = BeamMode::AUGMENTATOR;
		}
		else if (NewMode == BeamMode::AUGMENTATOR)
		{
			BeamState = BeamMode::DIMINUATOR;
		}
	}
	else
	{
		if (BeamState != NewMode)
		{
			BeamState = BeamMode::GRAB;
		}
		else
		{
			BeamState = BeamMode::OFF;
		}
	}
}

bool UBeamComponent::CheckScalingConditions(FVector newScale3D)
{
	return ((newScale3D.Size() > HitComponent->GetRelativeScale3D().Size() && !CheckScaleCollisions(HitComponent))
		|| newScale3D.Size() < HitComponent->GetRelativeScale3D().Size() && newScale3D.Size() > MinSize);
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
	
	case BeamMode::GRAB:
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
	return BeamState != BeamMode::OFF;
}

bool UBeamComponent::CheckScaleCollisions(UPrimitiveComponent* component)
{
	bool bUp1 = CheckVertexCollisions(component, FVector::UpVector, FVector::UpVector + FVector::LeftVector + FVector::ForwardVector);
	bool bUp2 = CheckVertexCollisions(component, FVector::UpVector, FVector::UpVector + FVector::LeftVector + FVector::BackwardVector);
	bool bUp3 = CheckVertexCollisions(component, FVector::UpVector, FVector::UpVector + FVector::RightVector + FVector::ForwardVector);
	bool bUp4 = CheckVertexCollisions(component, FVector::UpVector, FVector::UpVector + FVector::RightVector + FVector::BackwardVector);
	bool bUp = bUp1 || bUp2 || bUp3 || bUp4;

	bool bDown1 = CheckVertexCollisions(component, FVector::DownVector, FVector::DownVector + FVector::LeftVector + FVector::ForwardVector);
	bool bDown2 = CheckVertexCollisions(component, FVector::DownVector, FVector::DownVector + FVector::LeftVector + FVector::BackwardVector);
	bool bDown3 = CheckVertexCollisions(component, FVector::DownVector, FVector::DownVector + FVector::RightVector + FVector::ForwardVector);
	bool bDown4 = CheckVertexCollisions(component, FVector::DownVector, FVector::DownVector + FVector::RightVector + FVector::BackwardVector);
	bool bDown = bDown1 || bDown2 || bDown3 || bDown4;

	bool bLeft1 = CheckVertexCollisions(component, FVector::LeftVector, FVector::LeftVector + FVector::UpVector + FVector::ForwardVector);
	bool bLeft2 = CheckVertexCollisions(component, FVector::LeftVector, FVector::LeftVector + FVector::UpVector + FVector::BackwardVector);
	bool bLeft3 = CheckVertexCollisions(component, FVector::LeftVector, FVector::LeftVector + FVector::DownVector + FVector::ForwardVector);
	bool bLeft4 = CheckVertexCollisions(component, FVector::LeftVector, FVector::LeftVector + FVector::DownVector + FVector::BackwardVector);
	bool bLeft = bLeft1 || bLeft2 || bLeft3 || bLeft4;

	bool bRight1 = CheckVertexCollisions(component, FVector::RightVector, FVector::RightVector + FVector::UpVector + FVector::ForwardVector);
	bool bRight2 = CheckVertexCollisions(component, FVector::RightVector, FVector::RightVector + FVector::UpVector + FVector::BackwardVector);
	bool bRight3 = CheckVertexCollisions(component, FVector::RightVector, FVector::RightVector + FVector::DownVector + FVector::ForwardVector);
	bool bRight4 = CheckVertexCollisions(component, FVector::RightVector, FVector::RightVector + FVector::DownVector + FVector::BackwardVector);
	bool bRight = bRight1 || bRight2 || bRight3 || bRight4;

	bool bForward1 = CheckVertexCollisions(component, FVector::ForwardVector, FVector::ForwardVector + FVector::UpVector + FVector::LeftVector);
	bool bForward2 = CheckVertexCollisions(component, FVector::ForwardVector, FVector::ForwardVector + FVector::UpVector + FVector::RightVector);
	bool bForward3 = CheckVertexCollisions(component, FVector::ForwardVector, FVector::ForwardVector + FVector::DownVector + FVector::LeftVector);
	bool bForward4 = CheckVertexCollisions(component, FVector::ForwardVector, FVector::ForwardVector + FVector::DownVector + FVector::RightVector);
	bool bForward = bForward1 || bForward2 || bForward3 || bForward4;

	bool bBackward1 = CheckVertexCollisions(component, FVector::BackwardVector, FVector::BackwardVector + FVector::UpVector + FVector::LeftVector);
	bool bBackward2 = CheckVertexCollisions(component, FVector::BackwardVector, FVector::BackwardVector + FVector::UpVector + FVector::RightVector);
	bool bBackward3 = CheckVertexCollisions(component, FVector::BackwardVector, FVector::BackwardVector + FVector::DownVector + FVector::LeftVector);
	bool bBackward4 = CheckVertexCollisions(component, FVector::BackwardVector, FVector::BackwardVector + FVector::DownVector + FVector::RightVector);
	bool bBackward = bBackward1 || bBackward2 || bBackward3 || bBackward4;

	return (bUp && bDown) || (bLeft && bRight) || (bForward && bBackward);
}

bool UBeamComponent::CheckVertexCollisions(UPrimitiveComponent* component, const FVector direction, const FVector vertexDirection)
{
	FHitResult outHit;
	float cubeHalfLength = component->Bounds.SphereRadius / 2.0f;	// cube width
	float size = cubeHalfLength / 4;	// ray size
	FCollisionQueryParams params;
	params.AddIgnoredActor(component->GetOwner());
	//const FName TraceTag("TraceTag");
	//GetWorld()->DebugDrawTraceTag = TraceTag;
	//params.TraceTag = TraceTag;

	// Calculate vertex position by adding cube half length to the center in vertex direction.
	FVector start = component->GetComponentLocation() + component->GetComponentRotation().RotateVector(vertexDirection * cubeHalfLength);
	FVector end = start + component->GetComponentRotation().RotateVector(direction) * size;
	return GetWorld()->LineTraceSingleByChannel(outHit, start, end, ECollisionChannel::ECC_WorldStatic, params);
}
