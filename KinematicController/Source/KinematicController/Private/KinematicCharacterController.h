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
	VelocityVerlet,
	Verlet,
	RK4
};

struct KState
{
	FVector PositionDerivative;
	FVector VelocityDerivative;
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
	virtual void AsyncPhysicsTickActor(float DeltaTime, float SimTime) override;

	void ApplyVelocity(const float& DeltaTime);

	void CalculatePhysicsForces(const float& DeltaTime);


#pragma region TimeIntegrationMethods
	
	
	void CalculateEulerPosition(FVector& NewPosition, FVector& NewVelocity, const float& DeltaTime);

	/// <summary>
	/// Calculates position based on normal euler principle but instead of adding acceleration to velocity before calculation,
	/// it uses dt + 1/2at2 to get more accurate acceleration-based position and adds the accel to velocity afterwards
	/// </summary>
	void CalculateVelocityVerletPosition(FVector& NewPosition, FVector& NewVelocity, const float& DeltaTime);

	/// <summary>
	/// Calculates based on previous and current positions rather than by velocity, velocity is implied by the displacement between the points.
	/// </summary>
	/// <param name="PreviousPosition"></param>
	void CalculateVerletPosition(const FVector& PrevPos, FVector& NewPos, FVector& NewVelocity, const float& DeltaTime);
	/// <summary>
	/// Calculates the Position by averaging between start, midpoint 1, midpoint 2 and end to get more 
	/// accurate estimation but this is 4x more expensive. Stands for Runge-Kutta Method of the 4th Order
	/// Goddamn Ignoring this for now as I'm doing it wrong I think but nothing will tell me how or what I need to do
	/// Also isn't really necessary as currently acceleration is constant
	/// </summary>
	void CalculateRK4Position(FVector& NewPosition, FVector& NewVelocity, const float& DeltaTime);
	/// <summary>
	/// Calculates the Acceleration for the Runge Kutta 4th Order method
	/// RK4 method usually has variable acceleration in between timesteps to get an accurate calculation
	/// As in this kinematics only constant acceleration is used it is unneeded but a useful mathematical experience.
	/// </summary>
	/// <param name="Position"> Uses this to input the new decided Position</param>
	/// <param name="Velocity"> Uses this as reference for current velocity and to input new velocity</param>
	/// <param name="ComputedAccel"> Passes by reference the acceleration which is currently just the acceleration variable</param>
	void CalculateRK4Acceleration(const FVector& Position, const FVector& CurrentVelocity, FVector& ComputedAccel);
#pragma endregion
	/// <summary>
	/// Uses F = ma to get acceleration and transform into velocity
	/// </summary>
	/// <param name="AddedForce"> Used as base force or impulse and doesn't use const for the sake of the impulse </param>
	/// <param name="DeltaTime"> Turns acceleration into velocity and Impulse into force </param>
	/// <param name="TypeOfForce"> Checks whether the user wants the force to be an impulse or not</param>
	void AddForce(const FVector& AddedForce, const float& DeltaTime, const ForceType& TypeOfForce);

	FVector CollideAndSlideCollision(int& CurrentBounces, const FVector& CurrentVel, const FVector& InitialVel, FVector CurrentPos, const bool& IsGravity);
	
	inline float Square(const float& NumberToSquare);
	inline float Power(const float& MultNum, const int& Power);
#pragma region VectorMathematics

	/// <summary>
	/// Custom Function to get normalized Vector by dividing by magnitude
	/// </summary>
	/// <param name="FullVector"> Input Vector to be Normalized</param>
	/// <returns>Returns a normalized vector </returns>
	inline FVector Normalized(const FVector& FullVector);
	/// <summary>
	/// Custom Function to calculate magnitude of a vector by squaring and adding vector together then square rooting
	/// </summary>
	/// <param name="FullVector"> Vector to Find the magnitude of</param>
	/// <returns> Returns the magnitude of the inputted vector</returns>
	inline float Magnitude(const FVector& FullVector);

	inline float SquaredMagnitude(const FVector& FullVector);

	/// <summary>
	/// Projects the first vector onto the projection vector
	/// Using equation V . P/|P|sqr * P
	/// </summary>
	/// <param name="VectorToProject"> Vector that is projetced onto the projection vector</param>
	/// <param name="ProjectionVector"> Vector that is being projected on</param>
	/// <returns> Returns the projected vector</returns>
	inline FVector ProjectOnVector(const FVector& VectorToProject, const FVector& ProjectionVector);
	/// <summary>
	/// 
	/// </summary>
	/// <param name="FullVector"></param>
	/// <param name="PlaneNormal"></param>
	/// <returns></returns>
	inline FVector ProjectOnPlane(const FVector& FullVector, const FVector& PlaneNormal);

	inline float AngleBetweenVectors(const FVector& Vector1, const FVector& Vector2);

	inline float DotProduct(const FVector& FullVector, const FVector& VectorNormal);

	inline FVector CrossProduct(const FVector& Vector1, const FVector& Vector2);

	inline FVector ProjectAndScale(const FVector& FullVector, const FVector& PlaneNormal);

#pragma endregion

	FVector Acceleration = FVector(0);
	FVector PreviousAcceleration = FVector(0);

	FVector PreviousPosition = FVector(0);
	FVector Velocity = FVector(0);
	FVector GravityNormal = FVector::UpVector;
	float GravityMagnitude = -9.81f;
	int MaxBounces = 5;
	UPROPERTY(EditAnywhere)
	float Mass = 70.0f;

	UCapsuleComponent* Collider;

	float CapsuleHalfHeight = 90.0f;
	float CapsuleRadius = 30.0f;

	float SkinWidth = 0.02f;

	float MaxAngle = 80.0f;

	bool IsGrounded = false;

	UPROPERTY(EditAnywhere)
	USkeletalMesh* CharMesh;

	float FrictionCoefficent = 1.0f;

	float DragCoefficent = 0.1f;

	float MaxSpeed = 10.0f;

	float MoveSpeed = 5.0f;

	float JumpMagnitude = 20.0f;

	int MaxJumpCount = 1;
	int CurrentJumpCount = 0;

	FVector FloorNormal = FVector::UpVector;


	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
