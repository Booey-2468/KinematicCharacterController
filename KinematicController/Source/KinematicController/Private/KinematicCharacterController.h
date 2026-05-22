// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "Components/CapsuleComponent.h"

#include "KinematicCharacterController.generated.h"

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

	void AddAcceleration(const FVector& AddedAccelerationForce, const bool& IsAffectedByMass);
	/// <summary>
	/// Uses F = ma to get acceleration and transform into velocity
	/// </summary>
	/// <param name="AddedForce"> Used as base force or impulse and doesn't use const for the sake of the impulse </param>
	/// <param name="DeltaTime"> Turns acceleration into velocity and Impulse into force </param>
	/// <param name="IsImpulse"> Checks whether the user wants the force to be an impulse or not</param>
	void AddForce(FVector AddedForce, const float& DeltaTime, const bool& IsImpulse);

	void CollideAndSlideCollision(int& CurrentBounces, FVector& CurrentVel, FVector& InitialVel, );

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
