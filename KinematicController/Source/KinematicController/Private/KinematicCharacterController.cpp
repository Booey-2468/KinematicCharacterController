// Fill out your copyright notice in the Description page of Project Settings.


#include "KinematicCharacterController.h"

// Sets default values
AKinematicCharacterController::AKinematicCharacterController()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<UCapsuleComponent>("Character Collider");
	Collider = Cast<UCapsuleComponent>(RootComponent);
	Collider->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	USkeletalMeshComponent* skeleton = CreateDefaultSubobject<USkeletalMeshComponent>("Character Mesh");
	skeleton->SetSkeletalMesh(CharMesh);

}

// Called when the game starts or when spawned
void AKinematicCharacterController::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AKinematicCharacterController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

FVector AKinematicCharacterController::CollideAndSlideCollision(int& CurrentBounces, FVector& CurrentVel, FVector& InitialVel, FVector CurrentPos, const float& DeltaTime)
{
	if (CurrentBounces >= MaxBounces)
	{
		return FVector(0);
	}
	float dist = Magnitude(CurrentVel) + SkinWidth;

	dist *= DeltaTime;
	FHitResult hit;
	GetWorld()->DebugDrawTraceTag = "DebugLine";

	FCollisionQueryParams params;
	params.TraceTag = GetWorld()->DebugDrawTraceTag;

	GetWorld()->SweepSingleByChannel(hit, CurrentPos, CurrentPos + (dist * Normalized(CurrentVel)), GetActorQuat(), ECC_WorldStatic, Collider->GetCollisionShape(), params);

	if (hit.bBlockingHit)
	{
		FVector SnapToSurface = Normalized(CurrentVel) * (hit.Distance - SkinWidth);
	}

	return FVector(0);
}

inline FVector AKinematicCharacterController::Normalized(const FVector& FullVector)
{
	
	return FullVector/Magnitude(FullVector);
	
}

inline float AKinematicCharacterController::Magnitude(const FVector& FullVector)
{
	return FMath::Sqrt(FullVector.X * FullVector.X + FullVector.Y * FullVector.Y + FullVector.Z * FullVector.Z);
}

inline FVector AKinematicCharacterController::ProjectOnPlane(const FVector& FullVector, const FVector& PlaneNormal)
{
	DotPr
	return FVector();
}

inline FVector AKinematicCharacterController::ProjectOnVector(const FVector& VectorToProject, const FVector& ProjectionVector)
{
	return DotProduct(VectorToProject, ProjectionVector)/ Square(Magnitude(ProjectionVector)) * ProjectionVector;
}

inline float AKinematicCharacterController::DotProduct(const FVector& FullVector, const FVector& VectorNormal)
{
	return FullVector.X * VectorNormal.X + FullVector.Y * VectorNormal.Y + FullVector.Z * VectorNormal.Z;
}

inline float AKinematicCharacterController::Square(const float& NumberToSquare)
{
	return NumberToSquare * NumberToSquare;
}

inline FVector AKinematicCharacterController::CrossProduct(const FVector& Vector1, const FVector& Vector2)
{
	return FVector(Vector1.Y * Vector2.Z - Vector1.Z * Vector2.Y, Vector1.Z * Vector2.X - Vector1.X * Vector2.Z, Vector1.X * Vector2.Y - Vector1.Y * Vector2.X);
}

void AKinematicCharacterController::ApplyVelocity(const float& DeltaTime)
{
	FVector VerticalVelocity = GravityDir * FVector::DotProduct(Velocity, GravityDir);
	FVector HorizontalVelocity = Velocity - VerticalVelocity;

	FVector NewPosition;
	
	CalculateVelocityVerletPosition(NewPosition, Velocity, DeltaTime);

	PreviousPosition = GetActorLocation();

	SetActorLocation(NewPosition);

	Acceleration = FVector(0);
}

void AKinematicCharacterController::CalculateVelocityVerletPosition(FVector& NewPosition, FVector& NewVelocity, const float& DeltaTime)
{
	NewPosition += NewVelocity * DeltaTime + 0.5f * Acceleration * DeltaTime * DeltaTime;
	NewVelocity += Acceleration * DeltaTime;	// Needs to be done afterwards as Acceleration is seperately accounted for via velocity verlet equation
}

void AKinematicCharacterController::CalculateVerletPosition(const FVector& PreviousPosition, FVector& NewPosition, FVector& NewVelocity, const float& DeltaTime)
{
	FVector CurrentLocation = NewPosition;
	NewPosition = 2 * CurrentLocation - PreviousPosition + Acceleration * DeltaTime * DeltaTime;
	NewVelocity = (NewPosition - CurrentLocation)/DeltaTime;	// Using Forward difference as setting future velocity and don't need to get a larger average
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

// Called to bind functionality to input
void AKinematicCharacterController::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

