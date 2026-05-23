// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "Components/CapsuleComponent.h"

#include "KinematicCharacterController.generated.h"

enum ForceType
{
	Force,
	Acceleration,
	Impulse
};

enum TimeIntegration
{
	Euler,
	Verlet,
	RK4
};

UCLASS()
class AKinematicCharacterController : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AKinematicCharacterController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void ApplyVelocity(const float& DeltaTime);

	/// <summary>
	/// Calculates position based on normal euler principle but instead of adding acceleration to velocity before calculation,
	/// it uses dt + 1/2at2 to get more accurate acceleration-based position and adds the accel to velocity afterwards
	/// </summary>
	void CalculateEulerPosition(FVector& NewPosition, const float& DeltaTime);

	/// <summary>
	/// Calculates based on previous and current positions rather than by velocity, velcoity comes out as a byproduct.
	/// </summary>
	/// <param name="PreviousPosition"></param>
	void CalculateVerletPosition(const FVector& PreviousPosition, FVector& NewPosition, const float& DeltaTime);
	/// <summary>
	/// Calculates the Position by averaging between start, midpoint 1, midpoint 2 and end to get more 
	/// accurate estimation but this is 4x more expensive. Stands for Runge-Kutta Method of the 4th Order
	/// </summary>
	void CalculateRK4Position(const float& DeltaTime);

	void AddAcceleration(const FVector& AddedAccelerationForce, const bool& IsAffectedByMass);
	/// <summary>
	/// Uses F = ma to get acceleration and transform into velocity
	/// </summary>
	/// <param name="AddedForce"> Used as base force or impulse and doesn't use const for the sake of the impulse </param>
	/// <param name="DeltaTime"> Turns acceleration into velocity and Impulse into force </param>
	/// <param name="IsImpulse"> Checks whether the user wants the force to be an impulse or not</param>
	void AddForce(FVector AddedForce, const float& DeltaTime, const ForceType& TypeOfForce);

	void CollideAndSlideCollision(int& CurrentBounces, FVector& CurrentVel, FVector& InitialVel, FVector CurrentPos);

	FVector Acceleration;
	FVector PreviousVelocity;
	FVector Velocity = FVector(0);
	FVector GravityDir;
	FVector GravityMagnitude;
	UPROPERTY(EditAnywhere)
	float Mass = 70.0f;

	UCapsuleComponent* Collider;

	UPROPERTY(EditAnywhere)
	USkeletalMesh* CharMesh;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
