// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "GI_InputManager.h"

#include "Character.generated.h"

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
#pragma region PhysicsForceCalc

	void CalculatePhysicsForces();

	float CalculateNormalForce(const FVector& SurfaceNormal, const FVector& GravityDir, const float& GravityMag, const float& ObjMass, const float& FrictionCoeff);

	// Made the following 3 functions for RK4 integration and for when acceleration needs computing won't use in this project except maybe to make things looka bit cleaner
	
	FVector CalculateFrictionAccel(const FVector Vel, const FVector& SurfaceNormal, const FVector& GravityDir, const float& GravityMag, const float& ObjMass, const float& InvertedMass, const float& FrictionCoeff);

	FVector CalculateDragAccel(const FVector& Vel, const float& DragCoeff, const float& InvertedMass);

	FVector CalculateGravityAccel(const FVector& GravityDir, const float& GravityMag);

	/// <summary>
	/// Calculates The Impulse of a collision based on relative velocity, mass, restitution coefficent and the surface normal
	/// Only supports a collision between 2 objects at once. Adds 1 to e as to negate against all incoming velocity and then adds the extra bounce.
	/// </summary>
	/// <param name="RelativeVelocity"> Used to see how much relative velocity is moving away from collision</param>
	/// <param name="TotalMass"> The Mass of Both Objects Colliding</param>
	/// <param name="SurfaceNormal"> The surface normal the object is bouncing off of</param>
	/// <param name="Impulse"> The value to pass the resulting impulse to make sure to divide by mass to divy it up get the right ratio of the impulse</param>
	void CalculateBounceImpulse(const FVector& RelativeVelocity, const float& TotalMass, const FVector& SurfaceNormal, float& Impulse);
#pragma endregion



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
	void AddForce(const FVector& AddedForce, const ForceType& TypeOfForce);

	void AddTransformVel(const FVector& AddedTransform);

	FVector CollideAndSlideCollision(int& CurrentBounces, const FVector& CurrentVel, const FVector& InitialVel, FVector CurrentPos, const bool& IsGravity);
	
	inline float Square(const float& NumberToSquare);
	inline float Power(const float& MultNum, const int& Power);
	float Sqrt(const float& SqrtNum);
	float InvSqrt(const float& SqrtNum);
	inline void CalculateMomentum(const FVector& ObjVelocity, const float& ObjMass, FVector& ObjMomentum);
	/// <summary>
	/// Used to convert meters to centimeters as I prefer to use meters and its more common in physics overall
	/// Needed when interacting with any UE5 systems such as shape sweeps and setting actor location
	/// </summary>
	/// <param name="Vector"> The Vector to be Converted into UE5 cm units</param>
	/// <returns> The Converted Vector</returns>
	FVector ConvertToUE5Units(const FVector& Vector);
	/// <summary>
	/// Used to convert centimeters to meters as I prefer to use meters and its more common in physics overall
	/// Needed after getting UE5 Values such as actor location or hit result location/impact point
	/// </summary>
	/// <param name="Vector">The Vector to be Converted into normal m units </param>
	/// <returns></returns>
	FVector ConvertFromUE5Units(const FVector& Vector);
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

	/// <summary>
	/// Projects the first vector onto the projection vector
	/// Using equation V . P/|P|sqr * P
	/// </summary>
	/// <param name="VectorToProject"> Vector that is projetced onto the projection vector</param>
	/// <param name="ProjectionVector"> Vector that is being projected on</param>
	/// <returns> Returns the projected vector</returns>
	inline FVector ProjectOnVector(const FVector& VectorToProject, const FVector& ProjectionVector);
	/// <summary>
	/// Removes the Projection of Full vector on plane normal to get the vector part parallel to the normal
	/// </summary>
	/// <param name="FullVector"></param>
	/// <param name="PlaneNormal"></param>
	/// <returns></returns>
	inline FVector ProjectOnPlane(const FVector& FullVector, const FVector& PlaneNormal);

	inline float AngleBetweenVectors(const FVector& Vector1, const FVector& Vector2);

	/// <summary>
	/// Used to tell direction and magnitude towards a direction
	/// Also can serve as Squared Magnitude by inputting same vector for both parameters
	/// </summary>
	/// <param name="FullVector"></param>
	/// <param name="VectorNormal"></param>
	/// <returns></returns>
	inline float DotProduct(const FVector& FullVector, const FVector& VectorNormal);

	/// <summary>
	/// Used to Get the normal between 2 vectors
	/// </summary>
	/// <param name="Vector1"></param>
	/// <param name="Vector2"></param>
	/// <returns></returns>
	inline FVector CrossProduct(const FVector& Vector1, const FVector& Vector2);
	/// <summary>
	/// Uses Rodriguez' Rotation Formula to Rotate Vectors keeping their magnitude
	/// </summary>
	/// <param name="VectorToRotate"> The Vector that is rotated around the axis </param>
	/// <param name="Axis"> Needs to be a unit vector to work </param>
	/// <param name="Angle"> The Angle at which the vector needs to be rotated </param>
	/// <param name="IsDegrees"> If this is true the degrees are converted to radians as that is the input taken  by rodriguez' formula </param>
	/// <returns></returns>
	inline FVector RotateVector(const FVector& VectorToRotate, const FVector& Axis , float Angle, bool IsDegrees = true);

	/// <summary>
	/// Unrealistic Version of Project On Plane that keeps the same magnitude essentially rotating it 
	/// This is done to keep same velocity for the KCC for player experience but also means that I can't merge Movement Displacement and Gravity Displacement
	/// As this would mean gravity also pushes character movement which creates the issue of first hitting an object in movement displacement and which stunts jumps or falling as the player wouldn't be blocked if these were merged
	/// Not sure if I want to keep this to be honest
	/// </summary>
	/// <param name="FullVector"></param>
	/// <param name="PlaneNormal"></param>
	/// <returns></returns>
	inline FVector ProjectAndScale(const FVector& FullVector, const FVector& PlaneNormal);

#pragma endregion
	
	FVector Acceleration = FVector::ZeroVector;
	FVector PreviousAcceleration = FVector::ZeroVector;

	FVector PreviousPosition = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	FVector TransformVelocity = FVector::ZeroVector;
	FVector TotalImpulse = FVector::ZeroVector;
	float ImpulseDeltaTime = 0.02f;

	FVector GravityNormal = FVector::UpVector;
	float GravityMagnitude = -9.81f;

	float Mass = 70.0f;
	float InvMass = 0.0f;

	float CoefficientOfRestitution = 0.5f;

	float FrictionCoefficent = 0.5f;

	float DragCoefficent = 0.1f;

	FVector FloorNormal = FVector::UpVector;

	float MaxAngle = 80.0f;

	int MaxBounces = 10;

	bool IsGrounded = false;

	bool IsInContact = false;

	UCapsuleComponent* Collider;

	float CapsuleHalfHeight = 90.0f;
	float CapsuleRadius = 30.0f;

	float SkinWidth = 0.02f;

	UPROPERTY(EditAnywhere)
	USkeletalMesh* CharMesh;

	UGI_InputManager* InputManager;

	float MaxSpeed = 10.0f;

	float MoveSpeed = 5.0f;

	float JumpMagnitude = 20.0f;

	int MaxJumpCount = 1;
	int CurrentJumpCount = 0;



	UFUNCTION()
	void OnCharacterHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

#pragma region Player Input
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void MoveForward(float Axis, float DeltaTime);
	void MoveRight(float Axis, float DeltaTime);
	void JumpInput(float DeltaTime);

	void AddPlayerInputKeys();

	void AddMovementInput(AActor* MovementAxis);
#pragma endregion



};
