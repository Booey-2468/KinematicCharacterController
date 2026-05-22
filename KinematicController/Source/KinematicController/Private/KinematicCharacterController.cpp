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
void AKinematicCharacterController::AddAcceleration(const FVector& AddedAccelerationForce, const bool& IsAffectedByMass)
{
	if (IsAffectedByMass)
	{
		Acceleration += AddedAccelerationForce / Mass;
	}
	else
	{
		Acceleration += AddedAccelerationForce;
	}
}

void AKinematicCharacterController::AddForce(FVector AddedForce, const float& DeltaTime, const bool& IsImpulse)
{
	if (IsImpulse)	// Combined add force and impulse as impulse becomes a force after being divided by deltatime
	{
		AddedForce = AddedForce / DeltaTime;
	}
	Velocity += AddedForce / Mass * DeltaTime;
}

void AKinematicCharacterController::ApplyVelocity(const float& DeltaTime)
{
	FVector VerticalVelocity = GravityDir * FVector::DotProduct(Velocity, GravityDir);
	FVector HorizontalVelocity = Velocity - VerticalVelocity;

	FVector PlayerMovementDist = Velocity * DeltaTime;

	if (Acceleration.Length() > 0)
	{
		PlayerMovementDist += 0.5 * Acceleration * DeltaTime * DeltaTime;
		Velocity += Acceleration * DeltaTime;
	}	

	PlayerMovementDist += GetActorLocation();
	SetActorLocation(PlayerMovementDist);
}

// Called to bind functionality to input
void AKinematicCharacterController::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

