// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "GI_InputManager.h"

#include "CA_PlayerCamera.h"

#include "Components/TimelineComponent.h"

#include "EnhancedInput/Public/EnhancedInputSubsystems.h"

#include "Components/CapsuleComponent.h"

#include "EnhancedInputComponent.h"

#include "KCCPhysics.generated.h"

enum ForceType
{
	Force,
	Acceleration,
	Impulse
};

struct KState
{
	FVector PositionDerivative;
	FVector VelocityDerivative;
};
/// <summary>
/// Stores both stepping data that is used for stepping and correction vel allowing for smooth movement along slopes
/// </summary>
struct EditableCollideAndSlideData	// These are variables that are actively editable during Collide And Slide functions
{
	FHitResult StepHit;
	FVector RemainingVel = FVector::ZeroVector;
	FVector CorrectionVel = FVector::ZeroVector;
};
/// <summary>
/// 
/// </summary>
struct ConstantCollideAndSlideData	// While these are only set before collide and slide is called again
{
	FVector CurrentVel;
	FVector CurrentPos;
	FVector InitialVel;
	bool IsGravity;

	ConstantCollideAndSlideData(const FVector& CurrentVel, const FVector& CurrentPos, const FVector& InitialVel, const bool& IsGravity)
	{
		this->CurrentVel = CurrentVel;
		this->CurrentPos = CurrentPos;
		this->InitialVel = InitialVel;
		this->IsGravity = IsGravity;
	}
};

UCLASS()
class AKCCPhysics : public APawn
{
	GENERATED_BODY()

protected:
	// Sets default values for this pawn's properties
	AKCCPhysics();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/// <summary>
	/// Calls Every Physics Frame and Calls all physics and movement related Functions
	/// Use Super:: to Integrate forces with collision detection
	/// </summary>
	/// <param name="DeltaTime"> The amount of time that the physics frame covers </param>
	/// <param name="SimTime"> I do not know what this is </param>
	virtual void AsyncPhysicsTickActor(float DeltaTime, float SimTime) override;

#pragma region Physics Calc And Application

	FVector Acceleration = FVector::ZeroVector;		// This stores and accumulates accelertion until the end of the timestep where it is all then converted into velocity and position changes and this is then reset as the acceleration is not likely to be constant

	FVector PreviousAcceleration = FVector::ZeroVector;	// This stores the previous/current (depending on if you see the time step being acted in as the future or the current) timesteps acceleration and this is stores the current/future time steps acceleraion at the end of it. This is needed for certain time integrations that use previous acceleration for accuracy such as verlet and velocity verlet
	FVector PreviousPosition = FVector::ZeroVector;	// This stores the previous position of the player for time integration methods such as verlet

protected:
	FVector TransformVelocity = FVector::ZeroVector;	// This stores Transform Velocity which is essentially an acceleration that you don't want retained over time so it is a one time acceleration allowing for a simple transform based mpvement instead of physics based movement
	FVector TotalImpulse = FVector::ZeroVector;		// This stores the total impulse which is then added and multiplied by InvMass to velocity at the beginning of Apply Velocity this is mostly for unexpected collisions
	
public:
	FVector Velocity = FVector::ZeroVector;		// This stores and retains velocity this is applied during time integration but if interrupted via collisions then this is redefined by adding both displacements together and multiplying by inverse delta time
	
	UPROPERTY(EditAnywhere, Category = "KCC Physics")
	FVector GravityNormal = FVector::UpVector;		// This is the gravity normal and is being used as a normal as it makes comparisons to it a lot easier such as for friction
	
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, Units = "m/s^2"), Category = "KCC Physics")
	float GravityMagnitude = -9.81f;	// This is the gravity's magnitude and directly effects Normal force and hence friction

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, ClampMin = 0.0f, ClampMax = 100.0f), Category = "KCC Physics")
	float CoefficientOfRestitution = 0.5f;	// This is the ratio of kinetic enegy that would be lost during a bounce

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, ClampMin = 0.0f, ClampMax = 100.0f), Category = "KCC Physics")
	float FrictionCoefficent = 0.7f;	// This is the friction coefficient this decides how much friction the KCC has and how hard it is to get moving
	
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, ClampMin = 0.0f, ClampMax = 180.0f, Units = "m/s"), Category = "KCC Physics")
	float MinFrictionVel = 0.05f;	// This is the minimum velocity magnitude necessary for friction to act so it doesn't add more than the actual  velocity and set it in a constant state of vibration at rest

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, ClampMin = 0.0f, ClampMax = 100.0f), Category = "KCC Physics")
	float DragCoefficent = 0.4f;	// This is the drag coefficient and reduces velocity by a fraction of itself this happens due to air resistance or other forms of drag

protected:
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, ClampMin = 0.0f, ClampMax = 180.0f, Units = "Kilograms"), Category = "KCC Physics")
	float Mass = 70.0f;		// This represents the mass of the collider and is used for force, momentum and normal force calculations among others
	float InvMass = 1.0f;	// This is used to store the inverse Mass which increases efficiency as division is done once and then all further operations are multiplication which is less CPU heavy

public:
	/// <summary>
	/// Uses F = ma to get acceleration and transform into velocity
	/// </summary>
	/// <param name="AddedForce"> Used as base force or impulse and doesn't use const for the sake of the impulse </param>
	/// <param name="DeltaTime"> Turns acceleration into velocity and Impulse into force </param>
	/// <param name="TypeOfForce"> Checks whether the user wants the force to be an impulse or not</param>
	void AddForce(const FVector& AddedForce, const ForceType& TypeOfForce);

	/// <summary>
	/// This essentially just adds the vector to transform vel which is then multiplied by deltatime to distribute the movement evenly
	/// </summary>
	/// <param name="AddedTransform"> Added to TransformVel to be used in transform based movement </param>
	void AddTransformVel(const FVector& AddedTransform);
protected:

	/// <summary>
	/// This handles all the resulting physics, movement and collision handling as well as stepping
	/// </summary>
	/// <param name="DeltaTime"> Amount of time moved over time, also used for Time Integration</param>
	virtual void ApplyVelocity(const float& DeltaTime);

	/// <summary>
	/// This essentially applies all the linear physics forces as rotational physics isn't needing considered
	/// </summary>
	virtual void CalculatePhysicsForces();

	/// <summary>
	/// This calculates normal force which can be used for many things but in this case mainly for friction
	/// </summary>
	/// <param name="SurfaceNormal"> This is the surface normal that the force is applied from and decides what fraction of it is applied to oppose gravity </param>
	/// <param name="CurrentAccel"> This is the current Acceleration and is used to see how much acceleration is pushing against the surface that is in contact </param>
	/// <param name="ObjMass"> This is the Mass of the object that presses against the ground </param>
	/// <param name="FrictionCoeff"> This is the Coefficient of friction between the capsule and the ground </param>
	/// <returns> This returns the amount of normal force that resists gravity </returns>
	float CalculateNormalForce(const FVector& SurfaceNormal, const FVector& CurrentAccel, const float& ObjMass, const float& FrictionCoeff);
	
	// Made the following 3 functions for RK4 integration and for when acceleration needs computing won't use in this project except maybe to make things looka bit cleaner
	
	/// <summary>
	/// This Projects and normalizes the velocity onto the surface normal and of that velocity the normal force is calculated and mutliplied by that direction
	/// Calculates Friction for RK4 Acceleration calculations
	/// </summary>
	/// <param name="Vel"> This is the velocity that determines the direction of the friction</param>
	/// <param name="SurfaceNormal"> This is the Surface Normal that the friction moves along and scales friction lower the more inline with gravity</param>
	/// <param name="CurrentAccel"> This is the current Acceleration and is used to see how much acceleration is pushing against the surface that is in contact </param>
	/// <param name="ObjMass"> This is the mass of the object used for converting the normal force to a force</param>
	/// <param name="InvertedMass"> This is the inverted Mass which is used for turning the normal force into an acceleration</param>
	/// <param name="FrictionCoeff"> This is the coefficient of friction the KCC has</param>
	/// <returns> The acceleration vector of friction</returns>
	FVector CalculateFrictionAccel(const FVector Vel, const FVector& SurfaceNormal, const FVector& CurrentAccel, const float& ObjMass, const float& InvertedMass, const float& FrictionCoeff);

	/// <summary>
	/// This calculates the drag for the KCC
	/// Calculates drag for RK4 Acceleration calculations
	/// </summary>
	/// <param name="Vel"> This is used for the base of the drag </param>
	/// <param name="DragCoeff"> This is the drag coefficient use to determine what percentage of velocity is enacted as an opposing acceleration </param>
	/// <param name="InvertedMass"> This converts the drag force into an acceleration</param>
	/// <returns></returns>
	FVector CalculateDragAccel(const FVector& Vel, const float& DragCoeff, const float& InvertedMass);

	/// <summary>
	/// A simple gravity calculation for the KCC
	/// Calculates Gravity for RK4 Acceleration calculations
	/// </summary>
	/// <param name="GravityDir"> The direction of Gravity </param>
	/// <param name="GravityMag"> The Magnitude of Gravity </param>
	/// <returns> Gravity Acceleration </returns>
	FVector CalculateGravityAccel(const FVector& GravityDir, const float& GravityMag);

	/// <summary>
	/// Calculates the momentum of an object was previously in use for Impulse calculations but has now more been merged in impulse equation
	/// </summary>
	/// <param name="ObjVelocity"> The velocity the object is moving</param>
	/// <param name="ObjMass"> The mass of the object</param>
	/// <param name="ObjMomentum"> The momentum variable to be set</param>
	inline void CalculateMomentum(const FVector& ObjVelocity, const float& ObjMass, FVector& ObjMomentum);

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

#pragma region Collision Detection And Response
		
	FVector FloorNormal = FVector::UpVector;	// The floor normal allows things like friction to tell how much should be used and to tell if the player is on a slope

	UPROPERTY(EditAnywhere, meta=(AllowPrivateAccess = true, ClampMin = 0.0f, ClampMax = 180.0f, Units = "Degrees"), Category = "KCC Collision")
	float MaxSlopeAngle = 80.0f;	// The maximum slope angle where the slope is still treated like a floor instead of a slope

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, ClampMin = -1.0f, ClampMax = 1.0f), Category = "KCC Collision")
	float MinSlopeSimilarity = 0.7f;	// The Impact Normal hasn't always been accurate so this makes sure that both normals are semi similar

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, ClampMin = 0.0f, Units = "Meters"), Category = "KCC Collision")
	float MaxStepHeight = 1.0f;	// This is the max stepping height in meters

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, Units = "Meters"), Category = "KCC Collision")
	float MinStepDist = 0.2f;	// This is the minimum amount the capsule must be moved into the blocking surface so that the capsule doesn't immediately slide off the edge of the step

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, ClampMin = 0.0f, Units = "Meters"), Category = "KCC Collision")
	float GroundingCheck = 0.08f;	// This is added so that for gravity movement you get an extra skin width to avoid bumps that for some reasons occur on higher slopes

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 100), Category = "KCC Collision")
	int MaxBounces = 6;	// The maximum amount of bounces the Collide and Slide Algorithm can do before returning a zero vector this prevents infinite recursion

	bool IsGrounded = false;	// The boolean that shows if the player is grounded this is based off of the direction of the vertical movement and if there was a bounce in the collision

	bool IsSliding = false;	// This if true allows the player to slip off slopes utilizing gravity instead of cutting it off so you dn't always fall down slopes

	bool IsInContact = false;	// The boolean that says if there was any contact during collision and if friction should be applied

	UCapsuleComponent* Collider;	// The capsule Collider that stops unexpected collisions from phasing through

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, ClampMin = 1.0f, Units = "Centimeters"), Category = "KCC Collision")
	float CapsuleHalfHeight = 90.0f;	// The Capsule half height and radius in cm instead of meters

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, ClampMin = 1.0f, Units = "Centimeters"), Category = "KCC Collision")
	float CapsuleRadius = 30.0f;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, ClampMin = 0.0f, Units = "Meters"), Category = "KCC Collision")
	float SkinWidth = 0.02f;	// The skin width of the capsule collider in meters this is used heavily in the collision to make sure the collider never intersects with any of its environment
	
	/// <summary>
	/// This is the whole collision solution for the KCC it uses the collide and slide algorithm which slides the displacement projected onto the surface normal's plane
	/// There is currently a slight issue with bumps don't know why but it causes issues on higher slopes and moving against walls is speeding up the KCC too much.
	/// </summary>
	/// <param name="CurrentBounces"> This counts the current amount of bounces so the function knows when it should stop recursion and is used for grounding and contact logic.</param>
	/// <param name="CollisionData"> This is all the Essential data for the Collide and Slide function and is only changed when recursed. </param>
	/// <param name="SteppingInfo"> This is less essential but is used for stepping and correction vel which avoids some of the bouncing by removing it from velocity later on. </param>
	/// <returns> This returns the sum of all the snap to surface displacements along with the remaining vel that did not hit anything after being projected. </returns>
	virtual FVector CollideAndSlideCollision(int& CurrentBounces, const ConstantCollideAndSlideData& CollisionData , EditableCollideAndSlideData& SteppingInfo);

	/// <summary>
	/// This is the check used in the Collide and Slide Collision to when hitting walls using a sphere cast to check if it could be steppable or not.
	/// If there is enough distance for the capsule to fit it is then allowed and changed later
	/// </summary>
	/// <param name="SteppingInfo">This stores all the recieved information from the check if successful </param>
	/// <param name="Hit"> This stores the hit information such as mainly location to allow the </param>
	/// <param name="LeftoverVel"> This is the leftover velocity after the hit to make sure movement is not lost</param>
	/// <returns> This returns whether or not the step would be possible or not and if it is snap to surface is immediately returned in the Collision function. </returns>
	bool SteppingCheck(EditableCollideAndSlideData& SteppingInfo, const FHitResult& Hit, const FVector& LeftoverVel);

	/// <summary>
	/// This essentially just returns the new position with added leftover vel at the swept last valid location
	/// </summary>
	/// <param name="StepInfo"> This carries stepping information that is vital to getting the valid location and retaining velocity </param>
	/// <param name="CurrentDisplacement"> This allows changes to the displacement after it has been added to new position to retain velocity as the sweep already contains that displacement</param>
	/// <returns> Returns the valid location for stepping in UE5 units</returns>
	FVector SteppingLogic(const EditableCollideAndSlideData& StepInfo, FVector& CurrentDisplacement);

	/// <summary>
	/// This handles unexpected collisions such as with moving physics objects by checking for any penetration and moving out of it and otherwise just 
	/// applying a normal impulse to both the player and object if it is affected by physics
	/// </summary>
	/// <param name="HitComp"> This is the component that was hit during the collision which is obviously the collider </param>
	/// <param name="OtherActor"> This is the actor that was hit and is useful to check what class of actor was hit</param>
	/// <param name="OtherComp"> This is the component that was unexpectedly hit and is neccesary to see if an impulse can be applied to it </param>
	/// <param name="NormalImpulse"> This is Normal Impulse that would be applied to the other object normally</param>
	/// <param name="Hit"> This is the hit info and canpossibly show if it is penetrating and whether or not to do a penetration test </param>
	UFUNCTION()
	void OnCharacterHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	 
#pragma endregion


#pragma region TimeIntegrationMethods

	/// <summary>
	/// Calculates Position
	/// </summary>
	/// <param name="NewPosition"></param>
	/// <param name="NewVelocity"></param>
	/// <param name="DeltaTime"></param>
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
	/// This is just squaring code and can make some code such as the dot product more clean and readable
	/// </summary>
	/// <param name="NumberToSquare"> This is the number that needs multiplying by itself </param>
	/// <returns> The squared number</returns>
	inline float Square(const float& NumberToSquare);
	/// <summary>
	/// This is to the Power of code that uses a for loop to multiply numbers
	/// </summary>
	/// <param name="MultNum"> Number to be multiplied by itself many a time</param>
	/// <param name="Power"> Number that determines how many times MultNum is multiplied by itself</param>
	/// <returns> The Number to the power of Power</returns>
	inline float Power(const float& MultNum, const int& Power);


	float Sqrt(const float& SqrtNum);
	float InvSqrt(const float& SqrtNum);	// This was code that I wanted to create but realised CPU sqr rooting is much faster than one I create with code

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

	/// <summary>
	/// Used to convert meters to centimeters as I prefer to use meters and its more common in physics overall
	/// Needed when interacting with any UE5 systems such as shape sweeps and setting actor location
	/// </summary>
	/// <param name="Vector"> The Vector to be Converted into UE5 cm units</param>
	/// <returns> The Converted Vector</returns>
	float ConvertToUE5Units(const float& NumToConvert);
	/// <summary>
	/// Used to convert centimeters to meters as I prefer to use meters and its more common in physics overall
	/// Needed after getting UE5 Values such as actor location or hit result location/impact point
	/// </summary>
	/// <param name="Vector">The Vector to be Converted into normal m units </param>
	/// <returns></returns>
	float ConvertFromUE5Units(const float& NumToConvert);
#pragma region VectorMathematics

	/// <summary>
	/// Custom Function to get normalized Vector by dividing by magnitude
	/// </summary>
	/// <param name="FullVector"> Input Vector to be Normalized</param>
	/// <returns>Returns a normalized vector </returns>
	FVector SafeNormalized(const FVector& FullVector);

	/// <summary>
	/// Custom Function to get normalized Vector by dividing by magnitude
	/// </summary>
	/// <param name="FullVector"> Input Vector to be Normalized</param>
	/// <returns>Returns a normalized vector </returns>
	FVector UnSafeNormalized(const FVector& FullVector);
	/// <summary>
	/// Custom Function to calculate magnitude of a vector by squaring and adding vector together then square rooting
	/// </summary>
	/// <param name="FullVector"> Vector to Find the magnitude of</param>
	/// <returns> Returns the magnitude of the inputted vector</returns>
	float Magnitude(const FVector& FullVector);

	/// <summary>
	/// Projects the first vector onto the projection vector
	/// Using equation V.P/ P.P * P
	/// </summary>
	/// <param name="VectorToProject"> Vector that is projetced onto the projection vector</param>
	/// <param name="ProjectionVector"> Vector that is being projected on</param>
	/// <returns> Returns the projected vector</returns>
	FVector ProjectOnVector(const FVector& VectorToProject, const FVector& ProjectionVector);

	/// <summary>
	/// Projects the first vector onto the normal using dot product which is more efficient than the other version
	/// </summary>
	/// <param name="VectorToProject"> This is the vector to be projected onto the normal </param>
	/// <param name="ProjectionNormal"> This is the projection normal and must have a length of approx 1 or it will not give an accurate result</param>
	/// <returns> Returns the projected vector</returns>
	FVector ProjectOnNormal(const FVector& VectorToProject, const FVector& ProjectionNormal);
	/// <summary>
	/// Removes the Projection of Full vector on plane normal to get the vector part parallel to the normal
	/// </summary>
	/// <param name="FullVector"> This is the vector that is projected onto the plane </param>
	/// <param name="PlaneNormal"> This is the Vector that takes what is perpendicular to the plane</param>
	/// <returns> Returns the vector projected along the specified plane</returns>
	FVector ProjectOnPlane(const FVector& FullVector, const FVector& PlaneNormal);

	/// <summary>
	/// Removes the Projection of Full vector on plane normal to get the vector part parallel to the normal
	/// </summary>
	/// <param name="FullVector"> This is the vector that is projected onto the plane </param>
	/// <param name="PlaneNormal"> This is the Vector that takes what is perpendicular to the plane
	/// This must have an approx length of 1 as it saves efficiency by just using the dot product once with the normal</param>
	/// <returns> Returns the vector projected along the specified plane</returns>
	FVector ProjectOnNormalizedPlane(const FVector& FullVector, const FVector& PlaneNormal);

	/// <summary>
	/// Gets the total angle between 2 vectors
	/// </summary>
	/// <returns> The angle between the vectors </returns>
	float AngleBetweenVectors(const FVector& Vector1, const FVector& Vector2);

	/// <summary>
	/// Used to tell direction and magnitude towards a direction
	/// Also can serve as Squared Magnitude by inputting same vector for both parameters
	/// </summary>
	/// <param name="FullVector"></param>
	/// <param name="VectorNormal"></param>
	/// <returns></returns>
	float DotProduct(const FVector& FullVector, const FVector& VectorNormal);

	/// <summary>
	/// Used to Get the normal between 2 vectors
	/// </summary>
	/// <param name="Vector1"> The First Vector To Calculate the normal</param>
	/// <param name="Vector2"> The First Vector To Calculate the normal</param>
	/// <returns> The Normal between the 2 vectors </returns>
	FVector CrossProduct(const FVector& Vector1, const FVector& Vector2);
	/// <summary>
	/// Uses Rodriguez' Rotation Formula to Rotate Vectors keeping their magnitude
	/// </summary>
	/// <param name="VectorToRotate"> The Vector that is rotated around the axis </param>
	/// <param name="Axis"> Needs to be a unit vector to work </param>
	/// <param name="Angle"> The Angle at which the vector needs to be rotated </param>
	/// <param name="IsDegrees"> If this is true the degrees are converted to radians as that is the input taken  by rodriguez' formula </param>
	/// <returns></returns>
	FVector RotateVector(const FVector& VectorToRotate, const FVector& Axis , float Angle, bool IsDegrees = true);

	/// <summary>
	/// Basically how this works is you take away the vector that is going 
	/// </summary>
	/// <param name="VectorToReflect"></param>
	/// <param name="ReflectionNormal"></param>
	/// <returns></returns>
	FVector ReflectVectorOnNormal(const FVector& VectorToReflect, const FVector& ReflectionNormal);

	/// <summary>
	/// Unrealistic Version of Project On Plane that keeps the same magnitude essentially rotating it 
	/// This is done to keep same velocity for the KCC for player experience but also means that I can't merge Movement Displacement and Gravity Displacement
	/// Without separating them with ProjectOnPlane after hitting the first object considered a floor
	/// </summary>
	/// <param name="FullVector"> Is used for the scaling Magnitude and the direction to be projected along the plane</param>
	/// <param name="PlaneNormal"> This is used as the Normal to confine the vector to its corresponding plane, This does not need to be normalized</param>
	/// <returns> The Full Vector Projected and rescaled onto the plane</returns>
	FVector ProjectAndScale(const FVector& FullVector, const FVector& PlaneNormal);
	/// <summary>
	/// Unrealistic Version of Project On Plane that keeps the same magnitude essentially rotating it 
	/// This is done to keep same velocity for the KCC for player experience but also means that I can't merge Movement Displacement and Gravity Displacement
	/// Without separating them with ProjectOnPlane after hitting the first object considered a floor
	/// This version uses ProjectOnPlaneNormalized as this has much better performance but means you need to have a normalized plane
	/// </summary>
	/// <param name="FullVector"> Is used for the scaling Magnitude and the direction to be projected along the plane</param>
	/// <param name="PlaneNormal"> This is used as the Normal to confine the vector to its corresponding plane, This needs to be normalized</param>
	/// <returns></returns>
	FVector ProjectAndScaleNormalized(const FVector& FullVector, const FVector& PlaneNormal);

#pragma endregion

};
