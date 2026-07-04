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

	CapsuleRadius -= ConvertToUE5Units(SkinWidth) * 2;	// Converts To UE5 units as this is meant in meters and is applied as such

	Collider->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	Collider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collider->SetCollisionProfileName(TEXT("CustomKinematicCollision"));
	Collider->SetGenerateOverlapEvents(true);
	Collider->SetNotifyRigidBodyCollision(true);
	Collider->SetMaxDepenetrationVelocity(NAME_None, 0.0f);

	USkeletalMeshComponent* Skeleton = CreateDefaultSubobject<USkeletalMeshComponent>("Character Mesh");
	Skeleton->SetSkeletalMesh(CharMesh);
	Skeleton->SetupAttachment(RootComponent);
	Skeleton->SkeletalMesh = CharMesh;

}

// Called when the game starts or when spawned
void AKinematicCharacterController::BeginPlay()
{
	Super::BeginPlay();
	if (Collider)
	{
		Collider->OnComponentHit.AddDynamic(this, &AKinematicCharacterController::OnCharacterHit);
	}
	InvMass = 1 / Mass;	// Need to do again if mass is ever changed
}

void AKinematicCharacterController::AsyncPhysicsTickActor(float DeltaTime, float SimTime)
{
	if (Camera && PlayerController)
	{
		AddMovementInput(Camera);
		JumpTimerLogic(DeltaTime);
		JumpLogic();
	}

	CalculatePhysicsForces();
	ApplyVelocity(DeltaTime);
	if (PlayerController)
	{
		InputManager->TempResetKey(EKeys::W);
		InputManager->TempResetKey(EKeys::A);
		InputManager->TempResetKey(EKeys::S);
		InputManager->TempResetKey(EKeys::D);
	}

}
#pragma region Collision Detection And Response

FVector AKinematicCharacterController::CollideAndSlideCollision(int& CurrentBounces, const FVector& CurrentVel, const FVector& InitialVel, const FVector& CurrentPos, SteppingData& SteppingInfo,const bool& IsGravity)
{
	if (CurrentBounces >= MaxBounces || CurrentVel.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	float dist = Magnitude(CurrentVel) + SkinWidth;

	FHitResult Hit;

	GetWorld()->DebugDrawTraceTag = "DebugLine";
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.bTraceComplex = true;
	Params.TraceTag = GetWorld()->DebugDrawTraceTag;
	
	

	bool HasHit = GetWorld()->SweepSingleByChannel(Hit, ConvertToUE5Units(CurrentPos), ConvertToUE5Units(CurrentPos + (dist * Normalized(CurrentVel))), GetActorQuat(), ECC_WorldStatic, Collider->GetCollisionShape(), Params);

	if (HasHit)
	{
		FVector PreviousNormal = FloorNormal;
		FloorNormal = Hit.Normal;	// May need to use regular normal as impact normal is giving inaccurate results
		FVector SnapToSurface = Normalized(CurrentVel) * (ConvertFromUE5Units(Hit.Distance) - SkinWidth );
		// Takig away penetration depth slightly helps
		FVector LeftoverVelocity = CurrentVel - SnapToSurface;

		float Angle = AngleBetweenVectors(FloorNormal, GravityNormal);	// Nothing wrong with this

		if (Hit.bStartPenetrating)
		{
			SnapToSurface = Normalized(CurrentVel) * (ConvertFromUE5Units(Hit.Distance) - SkinWidth - ConvertFromUE5Units(Hit.PenetrationDepth));
		}
		if (LeftoverVelocity == FVector::ZeroVector)
		{
			++CurrentBounces;
			return SnapToSurface;
		}
		if (Angle <= MaxSlopeAngle)
		{
			if (IsGravity)	// If the check is for gravity this makes sure there is no sliding due to gravity
			{
				++CurrentBounces;	// Adding an extra bounce as am trying to tell whether the ground has been hit because of it and other than maybe slightly confusing the data it doesn't mess anything up
				return SnapToSurface;	// Could also add momentum and bounciness to this but would require another iteration of function
			}							// Optonally could add impulses to other objects if physics is enabled on them

			LeftoverVelocity = ProjectAndScale(LeftoverVelocity, FloorNormal);
		}
		else
		{
			bool CanStep = false;

			if (!IsGravity && IsGrounded)
				CanStep = SteppingCheck(SteppingInfo, Hit, LeftoverVelocity);	// The Stepping Check is done here in the Collision Detection and later dealt with after the displacemeant is done

			if (!CanStep)
			{
				FVector HitNormalXZ = Normalized(ProjectOnPlane(FloorNormal, GravityNormal));	// Added normalization at beginning as ProjectOnPlane needs it
				// 1 - limits dot product between 0 and 1

				FVector InitialVelXZ = ProjectOnPlane(InitialVel, GravityNormal);
				float Scale = 1.0f;

				if (Angle >= MinCreaseAngle && Magnitude(SnapToSurface) <= SkinWidth)	// Combined edge case
				{
					LeftoverVelocity = CurrentVel;
					SnapToSurface = FVector::ZeroVector;
				} // Attempt 1 at solving crease issue
				if (!InitialVelXZ.IsNearlyZero())	// Avoids normalizing InitialVelXZ when its 0 so there is no /0
				{
					Scale = 1 - DotProduct(HitNormalXZ, -Normalized(InitialVelXZ));
				}
				if (IsGrounded && !IsGravity)
				{				// Treats as flat wall if grounded and this is not the gravity check 
					LeftoverVelocity = ProjectAndScale(ProjectAndScale(LeftoverVelocity, GravityNormal), HitNormalXZ) * Scale;
					GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Residual Vel: " + FString::SanitizeFloat(Magnitude(LeftoverVelocity)));

					// Fixed by not normalizing the whole vector as scale is a decimal 0 - 1 scale
					// Has Issue of not removing velocity
				}
				else
				{
					LeftoverVelocity = ProjectAndScale(LeftoverVelocity, FloorNormal) * Scale;
				}				
			}
			else
			{
				++CurrentBounces;
				return SnapToSurface;
			}
			// Could also add momentum and bounciness to this by adding velocity from the impulse
			// Optonally could add impulses to other objects if physics is enabled on them
		}
		++CurrentBounces;	// Increments CurrentBounces
		return SnapToSurface + CollideAndSlideCollision(CurrentBounces, LeftoverVelocity, InitialVel, CurrentPos + SnapToSurface, SteppingInfo, IsGravity);
	}
	return CurrentVel;
}

bool AKinematicCharacterController::SteppingCheck(SteppingData& SteppingInfo, const FHitResult& Hit, const FVector& LeftoverVel)
{
	if (LeftoverVel.IsNearlyZero())
		return false;

	GetWorld()->DebugDrawTraceTag = "DebugLine";
	FCollisionQueryParams params;
	params.TraceTag = GetWorld()->DebugDrawTraceTag;
	params.AddIgnoredActor(this);

	FCollisionShape Sphere = FCollisionShape::MakeSphere(CapsuleRadius);

	FHitResult StepHit;

	float VelMag = Magnitude(LeftoverVel);

	const FVector VelDir = LeftoverVel / VelMag;

	VelMag = (VelMag < MinStepDist) ? MinStepDist : VelMag;

	FVector AddedSteppingDisp = Hit.Location + VelDir * ConvertToUE5Units(VelMag);

	FVector LowerSphere = AddedSteppingDisp - GravityNormal * (CapsuleHalfHeight - CapsuleRadius);

	FVector MaxHeight = LowerSphere + GravityNormal * (ConvertToUE5Units(MaxStepHeight) + (CapsuleHalfHeight - CapsuleRadius) * 2 + ConvertToUE5Units(SkinWidth));

	GetWorld()->SweepSingleByChannel(StepHit, MaxHeight, LowerSphere, GetActorQuat(), ECC_WorldStatic, Sphere, params);

	if (StepHit.bBlockingHit && !StepHit.bStartPenetrating && StepHit.Distance >= (CapsuleHalfHeight - CapsuleRadius) * 2 + ConvertToUE5Units(SkinWidth))
	{
		SteppingInfo.StepHit = StepHit;
		SteppingInfo.RemainingVel = LeftoverVel;
		return true;
	}
	return false;
}

FVector AKinematicCharacterController::SteppingLogic(const SteppingData& StepInfo, FVector& CurrentDisplacement)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "HasHit Stairs: " + FString::FromInt(StepInfo.StepHit.bBlockingHit && !StepInfo.StepHit.bStartPenetrating));
	CurrentDisplacement += StepInfo.RemainingVel;
	return StepInfo.StepHit.Location + GravityNormal * (CapsuleHalfHeight - CapsuleRadius + ConvertToUE5Units(SkinWidth));
}

void AKinematicCharacterController::OnCharacterHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherComp)
		return;
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
			Collider->MoveComponent(Depenetration, Collider->GetComponentRotation(), false);
		}
	}
	float VelocityImpulse;
	// In the end just needed to get the closest point on the component's collision to the player actor and base surface normal from that
	if (OtherComp->IsSimulatingPhysics())
	{
		// Some impulses can be very sudden and very large very weird but not the worst
		FVector OtherObjVelocity = ConvertFromUE5Units(OtherComp->GetPhysicsLinearVelocity());
		float OtherObjMass = OtherComp->GetMass();
		// Couldn't find how to reference restitution if ue5 physics material

		CalculateBounceImpulse(Velocity - OtherObjVelocity, Mass + OtherObjMass, SurfaceNormal, VelocityImpulse);
		FVector CharacterImpulse = VelocityImpulse * SurfaceNormal;		// Currently done along Impact Normal but may combine with projection method and change to a reflection of Impact Normal based on Initial velocity direction

		FVector OtherObjImpulse = VelocityImpulse * -SurfaceNormal;

		AddForce(CharacterImpulse, ForceType::Impulse);
		OtherComp->AddImpulseAtLocation(OtherObjImpulse, Hit.ImpactPoint);
	}
	else
	{
		CalculateBounceImpulse(Velocity, Mass, SurfaceNormal, VelocityImpulse);
		AddForce(VelocityImpulse * SurfaceNormal, ForceType::Impulse);
	}
}
#pragma endregion
#pragma region Physics Calc And Application

void AKinematicCharacterController::AddForce(const FVector& AddedForce, const ForceType& TypeOfForce)
{
	if (AddedForce.ContainsNaN())	// Ensures input Vectors don't contain unidentified or nigh infinite numbers and blocks entry if so
		return;
	else if (TypeOfForce == ForceType::Force)
	{
		Acceleration += AddedForce * InvMass;	// Changed From Velocity to Acceleration as a store as not all time Integration methods are based on velocity and allows for more accurate seperation
	}
	else if (TypeOfForce == ForceType::Impulse)	// Combined add force and impulse as impulse becomes a force after being divided by deltatime
	{
		TotalImpulse += AddedForce;
	}
	else if (TypeOfForce == ForceType::Acceleration)
	{
		Acceleration += AddedForce;
	}
}

void AKinematicCharacterController::AddTransformVel(const FVector& AddedTransform)
{
	if (AddedTransform.ContainsNaN())
		return;

	TransformVelocity += AddedTransform;
}

void AKinematicCharacterController::ApplyVelocity(const float& DeltaTime)
{
	float InvDeltaTime = 1 / DeltaTime;
	FVector NewPosition = ConvertFromUE5Units(GetActorLocation());

	Velocity += TotalImpulse * InvMass; // Changed Impulses to be applied directly to velocity as they are instant changes and p = MV and J = delta P/Momentum so I can just * InvMass 
	
	// Because of this I don't have to worry about how much of a timestep should it be divided by to be considered instantaneous
	CalculateVelocityVerletPosition(NewPosition, Velocity, DeltaTime);	

	PreviousPosition = ConvertFromUE5Units(GetActorLocation());

	int TotalBounces = 0;
	FVector TotalDisplacement = NewPosition - PreviousPosition;	// Decided to use new position instead of velocity so its not unneeded calculation also easier to add in transform movement options 
	// May be better to use actual displacement for other time integration methods but velocity verlet should be fairly accurate
	FVector GravityDisplacement = ProjectOnVector(TotalDisplacement, GravityNormal);

	float GravMag = Magnitude(GravityDisplacement);

	FVector MovementDisplacement = TotalDisplacement - GravityDisplacement;

	float OriginalMoveMag = Magnitude(MovementDisplacement);
	NewPosition = ConvertToUE5Units(PreviousPosition);

	SteppingData StepInfo;

	FloorNormal = FVector::ZeroVector;

	if (!TransformVelocity.IsNearlyZero())	// Removed Transform Velocity From
	{
		FVector TransformDisplacement = TransformVelocity * DeltaTime;
		TransformDisplacement = CollideAndSlideCollision(TotalBounces, TransformDisplacement, TransformDisplacement, PreviousPosition, StepInfo, false);
		NewPosition += ConvertToUE5Units(TransformDisplacement);
	}
	// Made so it adds to position as its velocity/displacement based rather than

	MovementDisplacement = CollideAndSlideCollision(TotalBounces, MovementDisplacement, MovementDisplacement, ConvertFromUE5Units(NewPosition), StepInfo, false);

	// Decided to do stepping here as otherwise you won't be able to tell what has been hit and where
	// Then will set actor location here and then have it equal NewPosition

	NewPosition += ConvertToUE5Units(MovementDisplacement);	// Now seperating displacement from new position so that it can also be used to change velocity

	NewPosition = (StepInfo.StepHit.bBlockingHit) ? SteppingLogic(StepInfo, MovementDisplacement) : NewPosition;

	int BouncesOnGround = TotalBounces;

	GravityDisplacement = CollideAndSlideCollision(BouncesOnGround, GravityDisplacement, GravityDisplacement, ConvertFromUE5Units(NewPosition), StepInfo, true);

	NewPosition += ConvertToUE5Units(GravityDisplacement);

	BouncesOnGround -= TotalBounces;
	TotalBounces += BouncesOnGround;

	// Checks if gravity displacement hit something and if gravity was going down towards the ground
	IsGrounded = (BouncesOnGround > 0 && DotProduct(ProjectOnVector(TotalDisplacement, GravityNormal), GravityNormal) < 0) ? true : false;
	IsInContact = (TotalBounces > 0) ? true : false;

		
	//if(Magnitude(MovementDisplacement) / OriginalMoveMag * 100 < 100)
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "MoveMagComparison:" + FString::FromInt(Magnitude(MovementDisplacement) / OriginalMoveMag * 100) + "%");
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Current Bounces:" + FString::FromInt(TotalBounces));

	if(IsInContact)	// Avoids extra calc and missing accuracy by redundantly calculating new velocity
		Velocity = (MovementDisplacement + GravityDisplacement) * InvDeltaTime;	// Updates Velocity based on collision displacement
	// Will have to use Movement Displacement for velocity though if stepping may temporarily stop the player
	// Used better way to solve issue instead of Manually adding radius and what not instead was able to get capsule location at collision for the displacement 
	// Snap To Surface now uses this which now doesn't overlap with anything

	SetActorLocation(NewPosition);

   	PreviousAcceleration = Acceleration;
	Acceleration = FVector::ZeroVector;
	TransformVelocity = FVector::ZeroVector;
	TotalImpulse = FVector::ZeroVector;
}

void AKinematicCharacterController::CalculatePhysicsForces()
{
	AddForce(CalculateGravityAccel(GravityNormal, GravityMagnitude), ForceType::Acceleration);

	float VelMag = Magnitude(Velocity);

	if (IsInContact && VelMag > 0.001f)	// Checks if there is any contact with a surface and if Velocity is large enough that friction doesn't spring it back and forth
	{
		// Changed what is usually Floor Normal to Gravity Normal to make movement more static as currently grvaity doesn't push down slopes so this just decreases friction unnecesarily
		FVector FrictionAccel = CalculateFrictionAccel(Velocity, FloorNormal, GravityNormal, GravityMagnitude, Mass, InvMass, FrictionCoefficent);
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Current Normal Force" + (-Normalized(ProjectOnPlane(Velocity, FloorNormal)) * NormalForce).ToCompactString());
		AddForce(FrictionAccel, ForceType::Acceleration);
	}
	AddForce(CalculateDragAccel(Velocity, DragCoefficent, InvMass), ForceType::Acceleration);
}

float AKinematicCharacterController::CalculateNormalForce(const FVector& SurfaceNormal, const FVector& GravityDir, const float& GravityMag, const float& ObjMass, const float& FrictionCoeff)
{
	return DotProduct(SurfaceNormal, GravityDir) * ObjMass * FMath::Abs(GravityMag) * FrictionCoeff;
}

FVector AKinematicCharacterController::CalculateFrictionAccel(const FVector Vel, const FVector& SurfaceNormal, const FVector& GravityDir, const float& GravityMag, const float& ObjMass, const float& InvertedMass, const float& FrictionCoeff)
{
	// Realized issue with KCC is that more worse surface normal means that gravity is going against both gravity more as well as friction
	return -Normalized(ProjectOnPlane(Vel, SurfaceNormal)) * CalculateNormalForce(SurfaceNormal, GravityDir, GravityMag, ObjMass, FrictionCoeff) * InvertedMass;
}

FVector AKinematicCharacterController::CalculateDragAccel(const FVector& Vel, const float& DragCoeff, const float& InvertedMass)
{
	return -Vel * DragCoeff * InvertedMass;
}

FVector AKinematicCharacterController::CalculateGravityAccel(const FVector& GravityDir, const float& GravityMag)
{
	return GravityDir * GravityMag;
}

inline void AKinematicCharacterController::CalculateMomentum(const FVector& ObjVelocity, const float& ObjMass, FVector& ObjMomentum)
{
	ObjMomentum = ObjVelocity * ObjMass;
}

void AKinematicCharacterController::CalculateBounceImpulse(const FVector& RelativeVelocity, const float& TotalMass, const FVector& SurfaceNormal , float& Impulse)
{
	Impulse = -(1 + CoefficientOfRestitution) * DotProduct(RelativeVelocity, SurfaceNormal);
	Impulse *= TotalMass;
}
#pragma endregion

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

FVector AKinematicCharacterController::ConvertToUE5Units(const FVector& Vector)
{
	return Vector * 100.0f;
}
FVector AKinematicCharacterController::ConvertFromUE5Units(const FVector& Vector)
{
	return Vector * 0.01f;	// Equivalent of 1/100 which is the equaivalent of /100 without the decreased performance
}
float AKinematicCharacterController::ConvertToUE5Units(const float& NumToConvert)
{
	return NumToConvert * 100.0f;
}
float AKinematicCharacterController::ConvertFromUE5Units(const float& NumToConvert)
{
	return NumToConvert * 0.01f;
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
	FVector Plane = ProjectOnPlane(FullVector, PlaneNormal);
	if (Plane.IsNearlyZero() || Plane.ContainsNaN())
		return FVector::ZeroVector;
	// This is not the cause there is some loss of extremely small vectors but nothing much else and doesn't sync with the staggering of the KCC
	return Normalized(Plane) * Magnitude(FullVector);
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
	NewVelocity += 0.5f * (PreviousAcceleration + Acceleration) * DeltaTime;	// Needs to be done afterwards as Acceleration is seperately accounted for via velocity verlet equation
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
#pragma region Player Input

void AKinematicCharacterController::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if ((PlayerController = Cast<APlayerController>(NewController)))
	{
		PlayerController->bAutoManageActiveCameraTarget = false;
		AddPlayerInputKeys();

		Camera = GetWorld()->SpawnActor<ACA_PlayerCamera>(ACA_PlayerCamera::StaticClass(), GetActorTransform());

		Camera->FocusedActor = this;

		PlayerController->SetViewTarget(Camera);

		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}
// Called to bind functionality to input
void AKinematicCharacterController::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Need to get local player to be able to get subsystem
	if (UEnhancedInputComponent* UserInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		UserInput->BindAction(MoveButton, ETriggerEvent::Triggered, this, &AKinematicCharacterController::Move);
		UserInput->BindAction(TurnCamAction, ETriggerEvent::Triggered, this, &AKinematicCharacterController::TurnCam);
		UserInput->BindAction(JumpButton, ETriggerEvent::Triggered, this, &AKinematicCharacterController::JumpInput);
	}
}

void AKinematicCharacterController::Move(const FInputActionValue& InputVal)
{
	FVector2D AxisVal = InputVal.Get<FVector2D>();	// Stores 2D WASD value
	InputKey* CurrentKey;
	if (AxisVal.Y > 0 && (CurrentKey = InputManager->GetInputKey(EKeys::W)))	// Checks AxisVal Y and if getting the input key is valid then it updates the key in the input manager
		InputManager->UpdateKeyData(CurrentKey->Key);

	else if(AxisVal.Y < 0 && (CurrentKey = InputManager->GetInputKey(EKeys::S)))
		InputManager->UpdateKeyData(CurrentKey->Key);	// Uses the worlds real time delta seconds as I can't get it from InputVal

	// Checks AxisVal X and if getting the input key is valid then it updates the key in the input manager
	if(AxisVal.X < 0 && (CurrentKey = InputManager->GetInputKey(EKeys::A)))	
		InputManager->UpdateKeyData(CurrentKey->Key);

	else if(AxisVal.X > 0 && (CurrentKey = InputManager->GetInputKey(EKeys::D)))
		InputManager->UpdateKeyData(CurrentKey->Key);
}

void AKinematicCharacterController::TurnCam(const FInputActionValue& InputVal)
{
	if (Camera)
	{
		Camera->CameraMovementAxis = InputVal.Get<FVector2D>();
	}
}

void AKinematicCharacterController::JumpInput(const FInputActionValue& InputVal)
{
	if (InputVal.Get<bool>())
	{
		InputManager->UpdateKeyData(EKeys::SpaceBar, GetWorld()->DeltaRealTimeSeconds);
	}
}

void AKinematicCharacterController::AddPlayerInputKeys()
{
	int MinimumInputFrames = 1;
	InputManager = Cast<UGI_InputManager>(GetGameInstance());
	InputManager->AddInputKey(EKeys::W, MinimumInputFrames);
	InputManager->AddInputKey(EKeys::S, MinimumInputFrames);
	InputManager->AddInputKey(EKeys::A, MinimumInputFrames);
	InputManager->AddInputKey(EKeys::D, MinimumInputFrames);
	InputManager->AddInputKey(EKeys::SpaceBar, MinimumInputFrames);
}

void AKinematicCharacterController::AddMovementInput(AActor* MovementAxis)
{
	FVector VelocityXZ = ProjectOnPlane(Velocity, GravityNormal);

	if (Magnitude(VelocityXZ) > MaxSpeed)
		return;

	FVector MovementForce = FVector::ZeroVector;
	FVector TransformForwardXZ = Normalized(ProjectOnPlane(MovementAxis->GetActorForwardVector(), GravityNormal));
	FVector TransformRightXZ = Normalized(ProjectOnPlane(MovementAxis->GetActorRightVector(), GravityNormal));

	
	InputKey* Key;

	if ((Key = InputManager->GetInputKey(EKeys::W)) && Key->HasBeenPressed)	// Tidied up so that GetInput Key isn't called twice
	{
		MovementForce += TransformForwardXZ;
	}
	if ((Key = InputManager->GetInputKey(EKeys::S)) && Key->HasBeenPressed)
	{
		MovementForce -= TransformForwardXZ;

	}
	if ((Key = InputManager->GetInputKey(EKeys::A)) && Key->HasBeenPressed)
	{
		MovementForce -= TransformRightXZ;

	}
	if ((Key = InputManager->GetInputKey(EKeys::D)) && Key->HasBeenPressed)
	{
		MovementForce += TransformRightXZ;
	}

	if (MovementForce.IsNearlyZero())	// Finally realised issue movement force is initially set to 0 and when normalizing you do 0/0 hence an infinite nan(ind) number
		return;

	MovementForce = Normalized(MovementForce);

	RotateToMovement(MovementForce);	 // Should Rotate player towards movement force

	FVector DriftForce = -ProjectOnPlane(VelocityXZ, MovementForce) * CorneringStiffness;
	//float SlopeScalingMod = (1 + (1 - DotProduct(FloorNormal, GravityNormal)) * SlopeMod);	// Added so slope can gain modifier when going up slopes

	MovementForce = MovementForce * MoveSpeed * CalculateSpeedMod(VelocityXZ, MovementForce);	// At this operation Movement force becomes a nan(ind) num since its added to accel accel becomes this to hence confusing the whole system
	// Should go roughly
	AddForce(MovementForce + DriftForce, ForceType::Acceleration);	// This for whatever reason is just disabling the physics no clue why
}
float AKinematicCharacterController::CalculateSpeedMod(const FVector& CurrentVelocity, const FVector& MovementDir)
{
	if (!SpeedCurve || !CorneringCurve)
		return 1.0f;

	float VelMag = Magnitude(CurrentVelocity);

	if (!IsGrounded)
	{
		return AirSpeed * CorneringCurve->FloatCurve.Eval(DotProduct(CurrentVelocity / VelMag, MovementDir) + 1);
	}
	return SpeedCurve->FloatCurve.Eval(VelMag / MaxSpeed) + CorneringCurve->FloatCurve.Eval(DotProduct(CurrentVelocity/VelMag, MovementDir) + 1);
}

void AKinematicCharacterController::JumpLogic()
{
	InputKey* Key = InputManager->GetInputKey(EKeys::SpaceBar);

	if (!Key)
		return;

	if (IsGrounded)
	{
		CurrentJumpCount = 0;
		JumpTimer = 0.0f;
		HasFallen = false;
	}

	bool CanJump = Key->HasBeenPressed || (JumpBufferTimer > 0.0f && IsGrounded) || (!IsGrounded && CoyoteTimer > 0.0f && Key->HasBeenPressed);

	CanJump = CanJump && CurrentJumpCount < MaxJumpCount;

	float UpwardVel = DotProduct(Velocity, GravityNormal);

	bool ShouldFall = JumpTimer > MinJumpTime && !Key->HasBeenPressed && !IsGrounded && !HasFallen && UpwardVel >= 0.0f;

	if (CanJump)
	{
		AddForce(JumpMagnitude * GravityNormal * Mass, ForceType::Impulse);
		++CurrentJumpCount;
		JumpTimer = 0.0f;
		Velocity -= ProjectOnVector(Velocity, GravityNormal);
	}
	else if (ShouldFall)
	{
    	AddForce(VariableHeightImp * -GravityNormal * Mass, ForceType::Impulse);
		HasFallen = true;
	}

	InputManager->TempResetKey(EKeys::SpaceBar);
	InputManager->OnKeyRelease(EKeys::SpaceBar);
	
}
void AKinematicCharacterController::JumpTimerLogic(const float& DeltaTime)
{
	if (IsGrounded)
	{
		CoyoteTimer = CoyoteTime;
	}
	else if (CoyoteTimer > 0.0f && !IsGrounded)
	{
		CoyoteTimer -= DeltaTime;
	}
	
	if(InputManager->GetInputKey(EKeys::SpaceBar)->HasBeenPressed && !IsGrounded)
	{
		JumpBufferTimer = JumpBufferTime;
	}
	else if (JumpBufferTimer > 0.0f)
	{
		JumpBufferTimer -= DeltaTime;
	}

	if (CurrentJumpCount > 0 && !IsGrounded)
	{
		JumpTimer += DeltaTime;
	}
}
void AKinematicCharacterController::RotateToMovement(const FVector& MovementVector)
{
	if (MovementVector.IsNearlyZero())
		return;
	FRotator MovementRotation = FQuat::Slerp(GetActorQuat(), Normalized(MovementVector).Rotation().Quaternion(), 1.0).Rotator();
	SetActorRotation(MovementRotation);
}
#pragma endregion