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
	Collider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collider->SetCollisionProfileName(TEXT("CustomKinematicCollision"));
	Collider->SetGenerateOverlapEvents(true);
	Collider->SetNotifyRigidBodyCollision(true);

	USkeletalMeshComponent* Skeleton = CreateDefaultSubobject<USkeletalMeshComponent>("Character Mesh");
	Skeleton->SetSkeletalMesh(CharMesh);
	Skeleton->SetupAttachment(RootComponent);

}

// Called when the game starts or when spawned
void AKinematicCharacterController::BeginPlay()
{
	Super::BeginPlay();
	if (Collider)
	{
		Collider->OnComponentHit.AddDynamic(this, &AKinematicCharacterController::OnCharacterHit);
	}
	AddPlayerInputKeys();
	InvMass = 1 / Mass;	// Need to do again if mass is ever changed
}

void AKinematicCharacterController::AsyncPhysicsTickActor(float DeltaTime, float SimTime)
{
	AddMovementInput(this, DeltaTime);
	CalculatePhysicsForces(DeltaTime);
	ApplyVelocity(DeltaTime);
}

void AKinematicCharacterController::AddForce(const FVector& AddedForce, const float& DeltaTime, const ForceType& TypeOfForce)
{
	if (TypeOfForce == ForceType::Force)
	{
		Acceleration += AddedForce * InvMass;	// Changed From Velocity to Acceleration as a store as not all time Integration methods are based on velocity and allows for more accurate seperation
	}
	else if (TypeOfForce == ForceType::Impulse)	// Combined add force and impulse as impulse becomes a force after being divided by deltatime
	{
		Acceleration = (AddedForce / DeltaTime) * InvMass;
	}
	else
	{
		Acceleration += AddedForce;
	}
}

void AKinematicCharacterController::AddTransformVel(const FVector& AddedTransform)
{
	TransformVelocity += AddedTransform;
}

FVector AKinematicCharacterController::CollideAndSlideCollision(int& CurrentBounces, const FVector& CurrentVel, const FVector& InitialVel, FVector CurrentPos, const bool& IsGravity)
{
	if (CurrentBounces >= MaxBounces)
	{
		return FVector::ZeroVector;
	}
	float dist = Magnitude(CurrentVel) + SkinWidth;

	FHitResult hit;

	GetWorld()->DebugDrawTraceTag = "DebugLine";
	FCollisionQueryParams params;
	params.TraceTag = GetWorld()->DebugDrawTraceTag;
	params.AddIgnoredActor(this);

	GetWorld()->SweepSingleByChannel(hit, ConvertToUE5Units(CurrentPos), ConvertToUE5Units(CurrentPos + (dist * Normalized(CurrentVel))), GetActorQuat(), ECC_WorldStatic, Collider->GetCollisionShape(), params);

	if (hit.bBlockingHit)
	{
		FloorNormal = hit.ImpactNormal;
		FVector SnapToSurface = Normalized(CurrentVel) * (Magnitude(ConvertFromUE5Units(hit.Location) - CurrentPos) - SkinWidth);
		FVector LeftoverVelocity = CurrentVel - SnapToSurface;

		float Angle = AngleBetweenVectors(hit.ImpactNormal, GravityNormal);


		if (Magnitude(SnapToSurface) <= SkinWidth)
		{
			SnapToSurface = FVector::ZeroVector;
		}
		if (Angle <= MaxAngle)
		{
			if (IsGravity)	// If the check is for gravity this makes sure there is no sliding due to gravity
			{
				if(CurrentBounces < 1)
					++CurrentBounces;	// Adding an extra bounce as am trying to tell whether the ground has been hit because of it and other than maybe slightly confusing the data it doesn't mess anything up
				return SnapToSurface;	// Could also add momentum and bounciness to this but would require another iteration of function
			}							// Optonally could add impulses to other objects if physics is enabled on them
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
			// Could also add momentum and bounciness to this by adding velocity from the impulse
			// Optonally could add impulses to other objects if physics is enabled on them
		}
		++CurrentBounces;	// Increments CurrentBounces
		return SnapToSurface + CollideAndSlideCollision(CurrentBounces, LeftoverVelocity, InitialVel, CurrentPos + SnapToSurface, IsGravity);
	}
	return CurrentVel;
}


void AKinematicCharacterController::ApplyVelocity(const float& DeltaTime)
{
	FVector NewPosition = ConvertFromUE5Units(GetActorLocation());
	
	CalculateVelocityVerletPosition(NewPosition, Velocity, DeltaTime);	

	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Current Velocity: " + Velocity.ToCompactString() + " Current Acceleration " + Acceleration.ToCompactString());

	PreviousPosition = ConvertFromUE5Units(GetActorLocation());

	int TotalBounces = 0;
	FVector TotalDisplacement = NewPosition - PreviousPosition;	// Decided to use new position instead of velocity so its not unneeded calculation also easier to add in transform movement options 
	TotalDisplacement += TransformVelocity * DeltaTime;	// Integrated transform velocity into total displacement
	// May be better to use actual displacement for other time integration methods but velocity verlet should be fairly accurate
	FVector GravityDisplacement = ProjectOnVector(TotalDisplacement, GravityNormal);
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Current Vertical Velocity: " + GravityDisplacement.ToCompactString() + " Current Acceleration " + Acceleration.ToCompactString());

	FVector MovementDisplacement = TotalDisplacement - GravityDisplacement;
	// Made so it adds to position as its velocity/displacement based rather than

	MovementDisplacement = CollideAndSlideCollision(TotalBounces, MovementDisplacement, MovementDisplacement, PreviousPosition, false);

	NewPosition = ConvertToUE5Units(PreviousPosition + MovementDisplacement);	// Now seperating displacement from new position so that it can also be used to change velocity
	int BouncesOnGround = TotalBounces;

	GravityDisplacement = CollideAndSlideCollision(BouncesOnGround, GravityDisplacement, GravityDisplacement, ConvertFromUE5Units(NewPosition), true);

	NewPosition += ConvertToUE5Units(GravityDisplacement);

	BouncesOnGround -= TotalBounces;
	TotalBounces += BouncesOnGround;

	// Checks if gravity displacement hit something and if gravity was going down towards the ground
	IsGrounded = (BouncesOnGround > 0 && DotProduct(ProjectOnVector(TotalDisplacement, GravityNormal), GravityNormal) < 0) ? true : false;
	IsInContact = (TotalBounces > 0) ? true : false;
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "TotalBounces" + FString::FromInt(TotalBounces));

	Velocity = (MovementDisplacement + GravityDisplacement) / DeltaTime;	// Updates Velocity based on collision displacement
	// Used better way to solve issue instead of Manually adding radius and what not instead was able to get capsule location at collision for the displacement 
	// Snap To Surface now uses this which now doesn't overlap with anything

	SetActorLocation(NewPosition);

	PreviousAcceleration = Acceleration;
	Acceleration = FVector::ZeroVector;
	TransformVelocity = FVector::ZeroVector;
}

void AKinematicCharacterController::CalculatePhysicsForces(const float& DeltaTime)
{
	AddForce(GravityNormal * GravityMagnitude, DeltaTime, ForceType::Acceleration);

	float VelMag = Magnitude(Velocity);

	if (IsInContact && VelMag > 0.001f)	// Checks if there is any contact with a surface and if Velocity is large enough that friction doesn't spring it back and forth
	{
		float NormalForce = DotProduct(FloorNormal, GravityNormal) * Mass * FMath::Abs(GravityMagnitude);
		NormalForce *= FrictionCoefficent;
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Current Normal Force" + (-Normalized(ProjectOnPlane(Velocity, FloorNormal)) * NormalForce).ToCompactString());
		AddForce(-Normalized(ProjectOnPlane(Velocity, FloorNormal)) * NormalForce, DeltaTime, ForceType::Force);
	}
	AddForce(-Velocity * DragCoefficent, DeltaTime, ForceType::Force);
}

void AKinematicCharacterController::AddPlayerInputKeys()
{
	int MinimumInputFrames = 5;
	InputManager = Cast<UGI_InputManager>(GetGameInstance());
	InputManager->AddInputKey(EKeys::W, MinimumInputFrames);
	InputManager->AddInputKey(EKeys::S, MinimumInputFrames);
	InputManager->AddInputKey(EKeys::A, MinimumInputFrames);
	InputManager->AddInputKey(EKeys::D, MinimumInputFrames);
	InputManager->AddInputKey(EKeys::SpaceBar, MinimumInputFrames);
}

void AKinematicCharacterController::AddMovementInput(AActor* MovementAxis, const float& DeltaTime)
{
	FVector MovementForce = FVector::ZeroVector;
	FVector TransformForwardXZ = ProjectOnPlane(ConvertFromUE5Units(MovementAxis->GetActorForwardVector()), GravityNormal);
	FVector TransformRightXZ = ProjectOnPlane(ConvertFromUE5Units(MovementAxis->GetActorRightVector()), GravityNormal);

	if (InputManager->GetInputKey(EKeys::W) && InputManager->GetInputKey(EKeys::W)->HasBeenPressed)
	{
		MovementForce += TransformForwardXZ * MoveSpeed;
	}
	if (InputManager->GetInputKey(EKeys::S) && InputManager->GetInputKey(EKeys::S)->HasBeenPressed)
	{
		MovementForce -= TransformForwardXZ * MoveSpeed;

	}
	if (InputManager->GetInputKey(EKeys::A) && InputManager->GetInputKey(EKeys::A)->HasBeenPressed)
	{
		MovementForce -= TransformRightXZ * MoveSpeed;

	}
	if (InputManager->GetInputKey(EKeys::D) && InputManager->GetInputKey(EKeys::D)->HasBeenPressed)
	{
		MovementForce += TransformRightXZ * MoveSpeed;
	}

	AddForce(MovementForce, DeltaTime, ForceType::Force);
}



void AKinematicCharacterController::OnCharacterHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Hit Result now holds something as I am using hit and have switched to blocking collision as to truly benefit from overlapping 
	// I would need to make my own physics engine and overlapping would've been fairly expensive comparitively even if I could do it
	FVector SurfaceNormal = Hit.ImpactNormal;
	if (Hit.bStartPenetrating)	// If the Hit was already penetrating from the beginning compute penetration
	{
		FMTDResult PenetrationResult;
		if (Collider->ComputePenetration(PenetrationResult, OtherComp->GetCollisionShape(), OtherComp->GetComponentLocation(), OtherComp->GetComponentQuat()))
		{
			FVector CurrentLocation = Collider->GetComponentLocation();
			FVector Depenetration = PenetrationResult.Direction * (PenetrationResult.Distance + 0.1f);
			// Moves Collider Comp with sweep to avoid further penetration
			Collider->MoveComponent(Depenetration, Collider->GetComponentQuat(), true);
		}
	}
	// Need to figure out penetration depth

	// In the end just needed to get the closest point on the component's collision to the player actor and base surface normal from that
	if (OtherComp->IsSimulatingPhysics())
	{
		FVector OtherObjVelocity = ConvertFromUE5Units(OtherComp->GetPhysicsLinearVelocity());
		float OtherObjMass = OtherComp->GetMass();
		// Couldn't find how to reference restitution if ue5 physics material

		float VelocityImpulse;
		
		CalculateBounceImpulse(Velocity - OtherObjVelocity, Mass + OtherObjMass, SurfaceNormal, VelocityImpulse);
		FVector CharacterImpulse = VelocityImpulse * SurfaceNormal;		// Currently done along Impact Normal but may combine with projection method and change to a reflection of Impact Normal based on Initial velocity direction

		FVector OtherObjImpulse = VelocityImpulse * -SurfaceNormal;

		AddForce(CharacterImpulse, ImpulseDeltaTime, ForceType::Impulse);
		OtherComp->AddImpulseAtLocation(OtherObjImpulse, Hit.ImpactPoint);
	}
	else
	{
		float VelocityImpulse;
		CalculateBounceImpulse(Velocity, Mass, SurfaceNormal, VelocityImpulse);

		AddForce(VelocityImpulse * SurfaceNormal, ImpulseDeltaTime, ForceType::Impulse);
	}
}

void AKinematicCharacterController::CalculateBounceImpulse(const FVector& RelativeVelocity, const float& TotalMass, const FVector& SurfaceNormal , float& TotalImpulse)
{
	TotalImpulse = -(1 + CoefficientOfRestitution) * DotProduct(RelativeVelocity, SurfaceNormal);
	TotalImpulse *= TotalMass;
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
inline void AKinematicCharacterController::CalculateMomentum(const FVector& ObjVelocity, const float& ObjMass, FVector& ObjMomentum)
{
	ObjMomentum = ObjVelocity * ObjMass;
}
FVector AKinematicCharacterController::ConvertToUE5Units(const FVector& Vector)
{
	return Vector * 100.0f;
}
FVector AKinematicCharacterController::ConvertFromUE5Units(const FVector& Vector)
{
	return Vector * 0.01f;	// Equivalent of 1/100 which is the equaivalent of /100 without the decreased performance
}
#pragma region VectorMathematics

inline FVector AKinematicCharacterController::Normalized(const FVector& FullVector)
{
	return FullVector / Magnitude(FullVector);
}

inline float AKinematicCharacterController::Magnitude(const FVector& FullVector)
{
	return FMath::Sqrt(DotProduct(FullVector, FullVector));
}

inline FVector AKinematicCharacterController::ProjectOnPlane(const FVector& FullVector, const FVector& PlaneNormal)
{
	return FullVector - ProjectOnVector(FullVector, PlaneNormal);
}

inline FVector AKinematicCharacterController::ProjectOnVector(const FVector& VectorToProject, const FVector& ProjectionVector)
{
	return DotProduct(VectorToProject, ProjectionVector) / DotProduct(ProjectionVector, ProjectionVector) * ProjectionVector;
}

inline float AKinematicCharacterController::DotProduct(const FVector& FullVector, const FVector& VectorNormal)
{
	return FullVector.X * VectorNormal.X + FullVector.Y * VectorNormal.Y + FullVector.Z * VectorNormal.Z;
}

inline FVector AKinematicCharacterController::CrossProduct(const FVector& Vector1, const FVector& Vector2)
{
	return FVector(Vector1.Y * Vector2.Z - Vector1.Z * Vector2.Y, Vector1.Z * Vector2.X - Vector1.X * Vector2.Z, Vector1.X * Vector2.Y - Vector1.Y * Vector2.X);
}

inline FVector AKinematicCharacterController::RotateVector(const FVector& VectorToRotate, const FVector& Axis, float Angle, bool IsDegrees)
{
	Angle = (IsDegrees) ? Angle * (PI/180): Angle;
	return VectorToRotate * FMath::Cos(Angle) + CrossProduct(Axis, VectorToRotate) * FMath::Sin(Angle) + Axis * DotProduct(Axis, VectorToRotate) * (1.0f - FMath::Cos(Angle));
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


