// Fill out your copyright notice in the Description page of Project Settings.


#include "KinematicCharacterController.h"

// Sets default values
AKinematicCharacterController::AKinematicCharacterController()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<UCapsuleComponent>("Character Collider");
	Collider = Cast<UCapsuleComponent>(RootComponent);
	Collider->SetCapsuleSize(20, 90);
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

void AKinematicCharacterController::AddForce(FVector AddedForce, const float& DeltaTime, const ForceType& TypeOfForce)
{
	if (TypeOfForce == ForceType::Force)
	{
		Acceleration += AddedForce / Mass;	// Changed From Velocity to Acceleration as a store as not all time Integration methods are based on velocity and allows for more accurate seperation
	}
	else if (TypeOfForce == ForceType::Impulse)	// Combined add force and impulse as impulse becomes a force after being divided by deltatime
	{
		AddedForce = AddedForce / DeltaTime;
	}
	else
	{
		Acceleration += AddedForce;
	}
}

void AKinematicCharacterController::ApplyVelocity(const float& DeltaTime)
{
	FVector VerticalVelocity = GravityDir * FVector::DotProduct(Velocity, GravityDir);
	FVector HorizontalVelocity = Velocity - VerticalVelocity;

	FVector NewPosition;

	CalculateEulerPosition(NewPosition, DeltaTime);
	SetActorLocation(NewPosition);

	Acceleration = FVector(0);
}

void AKinematicCharacterController::CalculateEulerPosition(FVector& NewPosition, const float& DeltaTime)
{
	NewPosition = Velocity * DeltaTime + 0.5 * Acceleration * DeltaTime * DeltaTime;
	NewPosition += GetActorLocation();
}

void AKinematicCharacterController::CalculateVerletPosition(const FVector& PreviousPosition, FVector& NewPosition, const float& DeltaTime)
{
	NewPosition = 2 * GetActorLocation() - PreviousPosition + Acceleration * DeltaTime * DeltaTime;
	Velocity += Acceleration * DeltaTime;	// Could Switch to Velocity verlet algorithm if expected Acceleration in between timesteps but decided not to out of unnecessary complexity and lack of reason to.
}

void AKinematicCharacterController::CalculateRK4Position(const float& DeltaTime)
{

}

// Called to bind functionality to input
void AKinematicCharacterController::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

