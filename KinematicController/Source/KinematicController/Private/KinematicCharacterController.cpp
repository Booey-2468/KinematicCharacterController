// Fill out your copyright notice in the Description page of Project Settings.


#include "KinematicCharacterController.h"

// Sets default values
AKinematicCharacterController::AKinematicCharacterController()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bAsyncPhysicsTickEnabled = true;

	RootComponent = CreateDefaultSubobject<UCapsuleComponent>("Character Collider");
	Collider = Cast<UCapsuleComponent>(RootComponent);

	CapsuleHalfHeight -= CapsuleRadius;
	CapsuleRadius -= SkinWidth;

	Collider->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	USkeletalMeshComponent* Skeleton = CreateDefaultSubobject<USkeletalMeshComponent>("Character Mesh");
	Skeleton->SetSkeletalMesh(CharMesh);
	Skeleton->SetupAttachment(RootComponent);

}

// Called when the game starts or when spawned
void AKinematicCharacterController::BeginPlay()
{
	Super::BeginPlay();
	
}

void AKinematicCharacterController::AsyncPhysicsTickActor(float DeltaTime, float SimTime)
{
	CalculatePhysicsForces(DeltaTime);
	ApplyVelocity(DeltaTime);
}

void AKinematicCharacterController::AddForce(const FVector& AddedForce, const float& DeltaTime, const ForceType& TypeOfForce)
{
	if (TypeOfForce == ForceType::Force)
	{
		Acceleration += AddedForce / Mass;	// Changed From Velocity to Acceleration as a store as not all time Integration methods are based on velocity and allows for more accurate seperation
	}
	else if (TypeOfForce == ForceType::Impulse)	// Combined add force and impulse as impulse becomes a force after being divided by deltatime
	{
		Acceleration = (AddedForce / DeltaTime)/Mass;
	}
	else
	{
		Acceleration += AddedForce;
	}
}

FVector AKinematicCharacterController::CollideAndSlideCollision(int& CurrentBounces, const FVector& CurrentVel, const FVector& InitialVel, FVector CurrentPos, const bool& IsGravity)
{
	if (CurrentBounces >= MaxBounces)
	{
		return FVector(0);
	}
	float dist = Magnitude(CurrentVel) + SkinWidth;

	FHitResult hit;

	GetWorld()->DebugDrawTraceTag = "DebugLine";
	FCollisionQueryParams params;
	params.TraceTag = GetWorld()->DebugDrawTraceTag;

	GetWorld()->SweepSingleByChannel(hit, CurrentPos, CurrentPos + (dist * Normalized(CurrentVel)), GetActorQuat(), ECC_WorldStatic, Collider->GetCollisionShape(), params);

	if (hit.bBlockingHit)
	{
		FVector SnapToSurface = Normalized(CurrentVel) * (Magnitude(hit.Location - CurrentPos) - SkinWidth);
		FVector LeftoverVelocity = CurrentVel - SnapToSurface;

		float Angle = AngleBetweenVectors(hit.ImpactNormal, GravityNormal);


		if (Magnitude(SnapToSurface) <= SkinWidth)
		{
			SnapToSurface = FVector(0);
		}
		if (Angle <= MaxAngle)
		{
			if (IsGravity)	// If the check is for gravity this makes sure there is no sliding due to gravity
			{
				return SnapToSurface;
			}
			LeftoverVelocity = ProjectAndScale(LeftoverVelocity, hit.ImpactNormal);
		}
		else
		{
			FVector HitNormalXZ = ProjectOnPlane(hit.ImpactNormal, GravityNormal);
			// 1 - limits dot product between 0 and 1
			float Scale = 1 - DotProduct(Normalized(HitNormalXZ), -Normalized(ProjectOnPlane(InitialVel, GravityNormal)));
			if (IsGrounded && !IsGravity)
			{
				LeftoverVelocity = Normalized(ProjectAndScale(ProjectOnPlane(LeftoverVelocity, GravityNormal), HitNormalXZ)) * Scale;
			}
			else
			{
				LeftoverVelocity = ProjectAndScale(LeftoverVelocity, hit.ImpactNormal) * Scale;
			}
		}
		++CurrentBounces;	// Increments CurrentBounces
		return SnapToSurface + CollideAndSlideCollision(CurrentBounces, LeftoverVelocity, InitialVel, CurrentPos + SnapToSurface, IsGravity);
	}

	// Added current pos as it's a displacement in reality but doesn't have attached current pos
	return CurrentVel;
}


void AKinematicCharacterController::ApplyVelocity(const float& DeltaTime)
{
	FVector NewPosition = GetActorLocation();
	
	CalculateEulerPosition(NewPosition, Velocity, DeltaTime);

	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Current Velocity: " + Velocity.ToCompactString() + " Current Acceleration " + Acceleration.ToCompactString());

	PreviousPosition = GetActorLocation();

	int TotalBounces = 0;
	
	// May be better to use actual displacement for other time integration methods but velocity verlet should be fairly accurate
	FVector GravityDisplacement = ProjectOnVector(Velocity, GravityNormal);
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Current Vertical Velocity: " + GravityDisplacement.ToCompactString() + " Current Acceleration " + Acceleration.ToCompactString());

	FVector MovementDisplacement = (Velocity - GravityDisplacement) * DeltaTime;
	GravityDisplacement *= DeltaTime;

	// Made so it adds to position as its velocity/displacement based rather than

	MovementDisplacement = CollideAndSlideCollision(TotalBounces, MovementDisplacement, MovementDisplacement, PreviousPosition, false);

	GravityDisplacement = CollideAndSlideCollision(TotalBounces, GravityDisplacement, GravityDisplacement, NewPosition, true);

	NewPosition += MovementDisplacement;	// Now seperating displacement from new position so that it can also be used to change velocity
	NewPosition += GravityDisplacement;

	Velocity = (MovementDisplacement + GravityDisplacement) / DeltaTime;	// Updates Velocity based on collision displacement

	// Used better way to solve issue instead of Manually adding radius and what not instead was able to get capsule location at collision for the displacement 
	// Snap To Surface now uses this which should not overlap with anything but for some reason its currently still phasing through the floor

	SetActorLocation(NewPosition);

	PreviousAcceleration = Acceleration;
	Acceleration = FVector(0);
}

void AKinematicCharacterController::CalculatePhysicsForces(const float& DeltaTime)
{
	AddForce(GravityNormal * GravityMagnitude, DeltaTime, ForceType::Acceleration);

	float VelMag = Magnitude(Velocity);

	if (IsGrounded && VelMag > 0.01f)
	{
		float NormalForce = DotProduct(FloorNormal, GravityNormal) * Mass * FMath::Abs(GravityMagnitude);
		NormalForce *= FrictionCoefficent;
		AddForce(-Normalized(ProjectOnPlane(Velocity, FloorNormal)) * NormalForce, DeltaTime, ForceType::Force);
	}
	if (VelMag > 0.01f)
	{
		AddForce(-Velocity * DragCoefficent, DeltaTime, ForceType::Force);
	}
}



// Called to bind functionality to input
void AKinematicCharacterController::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

inline float AKinematicCharacterController::Square(const float& NumberToSquare)
{
	return NumberToSquare * NumberToSquare;
}

inline float AKinematicCharacterController::Power(const float& MultNum, const int& Power)
{
	if (Power < 2)	return MultNum;	// Ensures that power isn't too low

	float PoweredNum = MultNum;	 // Stores for getting total number to the power of

	for (int i = 1; i < Power; ++i)	// Uses ++i as its minutely faster and skips over the current number
		PoweredNum *= MultNum;

	return PoweredNum;
}
#pragma region VectorMathematics

inline FVector AKinematicCharacterController::Normalized(const FVector& FullVector)
{
	return FullVector / Magnitude(FullVector);
}

inline float AKinematicCharacterController::Magnitude(const FVector& FullVector)
{
	return FMath::Sqrt(SquaredMagnitude(FullVector));
}

inline float AKinematicCharacterController::SquaredMagnitude(const FVector& FullVector)
{
	return Square(FullVector.X) + Square(FullVector.Y) + Square(FullVector.Z);
}

inline FVector AKinematicCharacterController::ProjectOnPlane(const FVector& FullVector, const FVector& PlaneNormal)
{
	return FullVector - ProjectOnVector(FullVector, PlaneNormal);
}

inline FVector AKinematicCharacterController::ProjectOnVector(const FVector& VectorToProject, const FVector& ProjectionVector)
{
	return DotProduct(VectorToProject, ProjectionVector) / SquaredMagnitude(ProjectionVector) * ProjectionVector;
}

inline float AKinematicCharacterController::DotProduct(const FVector& FullVector, const FVector& VectorNormal)
{
	return FullVector.X * VectorNormal.X + FullVector.Y * VectorNormal.Y + FullVector.Z * VectorNormal.Z;
}

inline FVector AKinematicCharacterController::CrossProduct(const FVector& Vector1, const FVector& Vector2)
{
	return FVector(Vector1.Y * Vector2.Z - Vector1.Z * Vector2.Y, Vector1.Z * Vector2.X - Vector1.X * Vector2.Z, Vector1.X * Vector2.Y - Vector1.Y * Vector2.X);
}

inline float AKinematicCharacterController::AngleBetweenVectors(const FVector& Vector1, const FVector& Vector2)
{
	return FMath::Acos(DotProduct(Vector1, Vector2) / (Magnitude(Vector1) * Magnitude(Vector2))) * (180 / PI);
}

inline FVector AKinematicCharacterController::ProjectAndScale(const FVector& FullVector, const FVector& PlaneNormal)
{
	return Normalized(ProjectOnPlane(FullVector, PlaneNormal)) * Magnitude(FullVector);
}
#pragma endregion
#pragma region TimeIntegrationMethods

void AKinematicCharacterController::CalculateEulerPosition(FVector& NewPosition, FVector& NewVelocity, const float& DeltaTime)
{
	NewVelocity += Acceleration * DeltaTime;
	NewPosition += NewVelocity * DeltaTime;
}

void AKinematicCharacterController::CalculateVelocityVerletPosition(FVector& NewPosition, FVector& NewVelocity, const float& DeltaTime)
{
	// Should technically use previous acceleration but might use acceleration to keep responsiveness
	NewPosition += NewVelocity * DeltaTime + 0.5f * PreviousAcceleration * DeltaTime * DeltaTime;
	NewVelocity += 0.5 * (PreviousAcceleration + Acceleration) * DeltaTime;	// Needs to be done afterwards as Acceleration is seperately accounted for via velocity verlet equation
}

void AKinematicCharacterController::CalculateVerletPosition(const FVector& PrevPos, FVector& NewPos, FVector& NewVelocity, const float& DeltaTime)
{
	FVector CurrentLocation = NewPos;
	NewPos = 2 * CurrentLocation - PrevPos + Acceleration * DeltaTime * DeltaTime;
	NewVelocity = (NewPos - CurrentLocation)/DeltaTime;	// Using Forward difference as setting future velocity and don't need to get a larger average
	// Symmetric Velocity Estimation would go over both timesteps as in previous -> current -> future and estimate based on that with double the deltatime
}

void AKinematicCharacterController::CalculateRK4Position(FVector& NewPosition, FVector& NewVelocity, const float& DeltaTime)
{	
	KState k1, k2, k3, k4;	// Stores current velocity

	// Accounts for y1 state to calculate k1
	FVector yPos = NewPosition;
	FVector yVel = NewVelocity;

	k1.PositionDerivative = yVel;
	CalculateRK4Acceleration(yPos, yVel, k1.VelocityDerivative);
	
	// Accounts for y2 state to calculate k3
	yPos = NewPosition + k1.PositionDerivative * 0.5f * DeltaTime;
	yVel = Velocity + k1.VelocityDerivative * 0.5f * DeltaTime;

	k2.PositionDerivative = yVel;
	CalculateRK4Acceleration(yPos, yVel, k2.VelocityDerivative);

	// Accounts for y2 state to calculate k3
	yPos = NewPosition + k2.PositionDerivative * 0.5f * DeltaTime;
	yVel = Velocity + k2.VelocityDerivative * 0.5f * DeltaTime;				// Think the issue is this all uses states and I decided to be stubborn and sort of do the same

	k3.PositionDerivative = yVel;
	CalculateRK4Acceleration(yPos, yVel, k3.VelocityDerivative);
	
	yPos = NewPosition + k3.PositionDerivative * DeltaTime;
	yVel = Velocity + k3.VelocityDerivative * DeltaTime;				// Think the issue is this all uses states and I decided to be stubborn and sort of do the same
	
	k4.PositionDerivative = yVel;
	CalculateRK4Acceleration(yPos, yVel, k4.VelocityDerivative);
	
	NewPosition = NewPosition + (k1.PositionDerivative + 2 * k2.PositionDerivative + 2 * k3.PositionDerivative + k4.PositionDerivative) * (DeltaTime/6.0f);

	NewVelocity += (k1.VelocityDerivative + 2 * k2.VelocityDerivative + 2 * k3.VelocityDerivative + k4.VelocityDerivative) * (DeltaTime/6.0f);
}

void AKinematicCharacterController::CalculateRK4Acceleration(const FVector& Position, const FVector& CurrentVelocity, FVector& ComputedAccel)
{
	ComputedAccel = Acceleration;
}

#pragma endregion


