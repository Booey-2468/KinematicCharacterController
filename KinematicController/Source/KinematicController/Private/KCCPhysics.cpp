// Fill out your copyright notice in the Description page of Project Settings.


#include "KCCPhysics.h"

// Sets default values
AKCCPhysics::AKCCPhysics()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bAsyncPhysicsTickEnabled = true;

	RootComponent = CreateDefaultSubobject<UCapsuleComponent>("Character Collider");
	Collider = Cast<UCapsuleComponent>(RootComponent);

	Collider->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	Collider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collider->SetCollisionProfileName(TEXT("CustomKinematicCollision"));
	Collider->SetGenerateOverlapEvents(true);
	Collider->SetNotifyRigidBodyCollision(true);
	Collider->SetMaxDepenetrationVelocity(NAME_None, 0.0f);
}

// Called when the game starts or when spawned
void AKCCPhysics::BeginPlay()
{
	Super::BeginPlay();
	if (Collider)
	{
		Collider->OnComponentHit.AddDynamic(this, &AKCCPhysics::OnCharacterHit);
	}
	InvMass = 1 / Mass;	// Need to do again if mass is ever changed
}

void AKCCPhysics::AsyncPhysicsTickActor(float DeltaTime, float SimTime)
{
	CalculatePhysicsForces();
	ApplyVelocity(DeltaTime);
}
#pragma region Collision Detection And Response

FVector AKCCPhysics::CollideAndSlideCollision(int& CurrentBounces, const ConstantCollideAndSlideData& CollisionData, EditableCollideAndSlideData& SteppingInfo)
{
	if (CurrentBounces >= MaxBounces)
	{
		return FVector::ZeroVector;
	}

	// Decided that for gravity checks I should add some distance for ground checking

	float dist = Magnitude(CollisionData.CurrentVel) + SkinWidth;
	float VelDot = DotProduct(CollisionData.CurrentVel, GravityNormal);

	if (CollisionData.IsGravity)
	{
		if (VelDot >= 0 && VelDot < SkinWidth)
			dist -= GroundingCheck;
		else if (VelDot <= 0)
			dist += GroundingCheck;
	}

	FHitResult Hit;

	//GetWorld()->DebugDrawTraceTag = "DebugLine";
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.bTraceComplex = true;
	//Params.TraceTag = GetWorld()->DebugDrawTraceTag;

	FCollisionShape ShapeBounds = FCollisionShape::MakeCapsule(CapsuleRadius - ConvertToUE5Units(SkinWidth), CapsuleHalfHeight - ConvertToUE5Units(SkinWidth));

	bool HasHit = GetWorld()->SweepSingleByChannel(Hit, ConvertToUE5Units(CollisionData.CurrentPos), ConvertToUE5Units(CollisionData.CurrentPos + (dist * SafeNormalized(CollisionData.CurrentVel))), GetActorQuat(), ECC_WorldStatic, ShapeBounds, Params);

	if (HasHit || Hit.bStartPenetrating)
	{
		float SnapDist = ConvertFromUE5Units(Hit.Distance) - SkinWidth;
		if (CollisionData.IsGravity)
		{
			if (VelDot >= 0 && VelDot < SkinWidth)
				SnapDist += GroundingCheck;
			else if (VelDot <= 0)
				SnapDist -= GroundingCheck;
		}

		FVector SnapToSurface = SafeNormalized(CollisionData.CurrentVel) * SnapDist;

		FVector LeftoverVelocity = CollisionData.CurrentVel - SnapToSurface;

		/*else if (!CollisionData.IsGravity && SnapDist <= SkinWidth)
		{
			SteppingInfo.CorrectionVel += SnapToSurface;
		}*/
		// Ok Next thing to do is check 2 things one whether snap to surface is causing the issues or is it current vel if current vel
		// I dont believe thats something I may be able to fix otherwise I can hopefully find a way

		if (DotProduct(Hit.ImpactNormal, Hit.Normal) > MinSlopeSimilarity)	// Checks whether the impact normal and normal are fairly similar if so the impact normal can be trusted to use
			FloorNormal = Hit.ImpactNormal;
		else
			FloorNormal = Hit.Normal;	// This is good likely doesn't need changing

		/*float SnapDot = DotProduct(SnapToSurface, FloorNormal);	// This reverted to what it was when adding that snap dist must be bigger than 0 meaning it is the corrrection code that may indeed be causing the issue

		if (SnapDist <= SkinWidth && SnapDot >= 0.0f)
		{
			SnapToSurface -= SnapDot * FloorNormal;
			LeftoverVelocity += SnapDot * FloorNormal;
		}*/

		if (SnapDist <= 0.0f && !Hit.bStartPenetrating)	// Avoids excess correction vel but this is creating an issue going against walls
		{
			SteppingInfo.CorrectionVel += SnapToSurface;	// Might No longer add as it may create too much correction vel
		}

		float Angle = AngleBetweenVectors(FloorNormal, GravityNormal);	

		//if (DotProduct(SnapToSurface, FloorNormal) > 0)
			//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Bad issue:" + FString::SanitizeFloat(DotProduct(SnapToSurface, FloorNormal)));
		if (Hit.bStartPenetrating)
		{
			SnapToSurface = FloorNormal * (ConvertFromUE5Units(Hit.PenetrationDepth) + SkinWidth);	// Removed Hit distance in case it wasn't set to 0 when penetrating
			LeftoverVelocity = CollisionData.CurrentVel;
			SteppingInfo.CorrectionVel += SnapToSurface;
		}

		if (Angle <= MaxSlopeAngle)
		{
			if (CollisionData.IsGravity && !IsSliding)	// If the check is for gravity this makes sure there is no sliding due to gravity
			{
				++CurrentBounces;	// Adding an extra bounce as am trying to tell whether the ground has been hit because of it and other than maybe slightly confusing the data it doesn't mess anything up
				return SnapToSurface;	// Could also add momentum and bounciness to this but would require another iteration of function
			}							// Optonally could add impulses to other objects if physics is enabled on them
		
			//float LeftOverMag = Magnitude(LeftoverVelocity);

			LeftoverVelocity = ProjectAndScaleNormalized(LeftoverVelocity, FloorNormal);
			//if(Magnitude(LeftoverVelocity) / LeftOverMag * 100 < 99)
				//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Vel Percent: " + FString::SanitizeFloat(LeftOverMag) + "%");
		}
		else
		{
			bool CanStep = false;

			if (!CollisionData.IsGravity && IsGrounded)
				CanStep = SteppingCheck(SteppingInfo, Hit, LeftoverVelocity);	// The Stepping Check is done here in the Collision Detection and later dealt with after the displacemeant is done

			if (!CanStep)
			{
				FVector HitNormalXZ = SafeNormalized(ProjectOnNormalizedPlane(FloorNormal, GravityNormal));	// Added normalization at beginning as ProjectOnPlane needs it

				// 1 - limits dot product between 0 and 1

				FVector InitialVelXZ = ProjectOnNormalizedPlane(CollisionData.InitialVel, GravityNormal);
				
				float Scale = 1 - DotProduct(HitNormalXZ, -SafeNormalized(InitialVelXZ));
			
				if (IsGrounded && !CollisionData.IsGravity)
				{				// Treats as flat wall if grounded and this is not the gravity check 
					LeftoverVelocity = ProjectAndScaleNormalized(ProjectOnNormalizedPlane(LeftoverVelocity, GravityNormal), HitNormalXZ) * Scale;
					// Fixed by not normalizing the whole vector as scale is a decimal 0 - 1 scale
					// Has Issue of not removing velocity
				}
				else
				{
					LeftoverVelocity = ProjectAndScaleNormalized(LeftoverVelocity, FloorNormal) * Scale;
				}

				if (Hit.Component->IsSimulatingPhysics())	// Added some code so that when hitting a wall that is considered a physics object it should add the appropriate impulse
				{
					float Impulse;
					CalculateBounceImpulse(ConvertToUE5Units(Velocity) - Hit.Component->GetComponentVelocity(), Hit.Component->GetMass() + Mass, FloorNormal, Impulse);
					Hit.Component->AddImpulseAtLocation(TotalImpulse * -FloorNormal, Hit.ImpactPoint);
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
		ConstantCollideAndSlideData CollisionInfo = ConstantCollideAndSlideData(LeftoverVelocity, CollisionData.CurrentPos + SnapToSurface, CollisionData.InitialVel, CollisionData.IsGravity);
		return SnapToSurface + CollideAndSlideCollision(CurrentBounces, CollisionInfo, SteppingInfo);
	}
	return CollisionData.CurrentVel;
}

bool AKCCPhysics::SteppingCheck(EditableCollideAndSlideData& SteppingInfo, const FHitResult& Hit, const FVector& LeftoverVel)
{
	if (LeftoverVel.IsNearlyZero())
		return false;

	//GetWorld()->DebugDrawTraceTag = "DebugLine";
	FCollisionQueryParams params;
	//params.TraceTag = GetWorld()->DebugDrawTraceTag;
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

FVector AKCCPhysics::SteppingLogic(const EditableCollideAndSlideData& StepInfo, FVector& CurrentDisplacement)
{
	if(TransformVelocity.IsNearlyZero())
		CurrentDisplacement += StepInfo.RemainingVel;
	return StepInfo.StepHit.Location + GravityNormal * (CapsuleHalfHeight - CapsuleRadius + ConvertToUE5Units(SkinWidth));
}

void AKCCPhysics::OnCharacterHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
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

void AKCCPhysics::AddForce(const FVector& AddedForce, const ForceType& TypeOfForce)
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

void AKCCPhysics::AddTransformVel(const FVector& AddedTransform)
{
	if (AddedTransform.ContainsNaN())
		return;

	TransformVelocity += AddedTransform;
}

void AKCCPhysics::ApplyVelocity(const float& DeltaTime)
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
	FVector GravityDisplacement = ProjectOnNormal(TotalDisplacement, GravityNormal);

	FVector MovementDisplacement = TotalDisplacement - GravityDisplacement;

	float OriginalMoveMag = Magnitude(MovementDisplacement);
	NewPosition = ConvertToUE5Units(PreviousPosition);

	EditableCollideAndSlideData StepInfo;

	FloorNormal = FVector::ZeroVector;

	if (!TransformVelocity.IsNearlyZero())	// Removed Transform Velocity From
	{
		FVector TransformDisplacement = TransformVelocity * DeltaTime;

		ConstantCollideAndSlideData CollisionInfo = ConstantCollideAndSlideData(TransformDisplacement, PreviousPosition, TransformDisplacement, false);

		TransformDisplacement = CollideAndSlideCollision(TotalBounces, CollisionInfo, StepInfo);
		NewPosition += ConvertToUE5Units(TransformDisplacement);

		StepInfo.CorrectionVel = FVector::ZeroVector;
	}
	// Made so it adds to position as its velocity/displacement based rather than

	ConstantCollideAndSlideData CollisionInfo = ConstantCollideAndSlideData(MovementDisplacement, ConvertFromUE5Units(NewPosition), MovementDisplacement, false);

	MovementDisplacement = CollideAndSlideCollision(TotalBounces, CollisionInfo, StepInfo);

	// Decided to do stepping here as otherwise you won't be able to tell what has been hit and where
	// Then will set actor location here and then have it equal NewPosition

	NewPosition += ConvertToUE5Units(MovementDisplacement);	// Now seperating displacement from new position so that it can also be used to change velocity

	MovementDisplacement -= StepInfo.CorrectionVel;
	StepInfo.CorrectionVel = FVector::ZeroVector;

	NewPosition = (StepInfo.StepHit.bBlockingHit) ? SteppingLogic(StepInfo, MovementDisplacement) : NewPosition;

	int BouncesOnGround = TotalBounces;

	CollisionInfo = ConstantCollideAndSlideData(GravityDisplacement, ConvertFromUE5Units(NewPosition), GravityDisplacement, true);
	
	GravityDisplacement = CollideAndSlideCollision(BouncesOnGround, CollisionInfo, StepInfo);

	NewPosition += ConvertToUE5Units(GravityDisplacement);

	GravityDisplacement -= StepInfo.CorrectionVel;

	BouncesOnGround -= TotalBounces;
	TotalBounces += BouncesOnGround;

	// Checks if gravity displacement hit something and if gravity was going down towards the ground
	IsGrounded = (BouncesOnGround > 0 && DotProduct(GravityDisplacement, GravityNormal) <= 0) ? true : false;
	IsInContact = (TotalBounces > 0) ? true : false;


	// Ok so my Movement with TransformVel is somewhat fine which means the issue is indeed velocity and possibly how my Collision Detection can affect it
	// It seems that the Collision Detection may have an issue with smaller Velocities
	
	//if(DotProduct(MovementDisplacement, GravityNormal) > 0 && DotProduct(TotalDisplacement, GravityNormal) < 0)
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Current Accel:" + FString::SanitizeFloat(DotProduct(GravityDisplacement, GravityNormal)) + " " + FString::SanitizeFloat(DotProduct(TotalDisplacement, GravityNormal)));
	// KCC is not grounded at times causing Speed mod to be 0.3f

	// Ok so clearly the issue is being produced by movement displacement adding Vertical height

	//if (Magnitude(MovementDisplacement) / OriginalMoveMag * 100 < 99)
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "MoveMagComparison:" + FString::FromInt(Magnitude(MovementDisplacement) / OriginalMoveMag * 100) + "%");
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Current Bounces:" + FString::FromInt(BouncesOnGround));

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

void AKCCPhysics::CalculatePhysicsForces()
{
	AddForce(CalculateGravityAccel(GravityNormal, GravityMagnitude), ForceType::Acceleration);

	float VelMag = Magnitude(Velocity);

	AddForce(CalculateDragAccel(Velocity, DragCoefficent, InvMass), ForceType::Acceleration);

	// Now scales min friction vel by friction coefficient and uses a larger value to stop jitter
	if (IsInContact && VelMag > MinFrictionVel * FrictionCoefficent)	// Checks if there is any contact with a surface and if Velocity is large enough that friction doesn't spring it back and forth
	{
		// Changed what is usually Floor Normal to Gravity Normal to make movement more static as currently grvaity doesn't push down slopes so this just decreases friction unnecesarily
		FVector FrictionAccel = CalculateFrictionAccel(Velocity, GravityNormal, Acceleration, Mass, InvMass, FrictionCoefficent);
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Current Normal Force" + (-SafeNormalized(ProjectOnPlane(Velocity, FloorNormal)) * NormalForce).ToCompactString());
		AddForce(FrictionAccel, ForceType::Acceleration);
	}
}

float AKCCPhysics::CalculateNormalForce(const FVector& SurfaceNormal, const FVector& CurrentAccel, const float& ObjMass, const float& FrictionCoeff)
{
	// Changed normal force to be more accurate so any forces moving into the surface are included in normal force calc but normal force can't be negative so I check that it isn't and invert the surface normal for the dot product
	float AccelInSurface = DotProduct(CurrentAccel, -SurfaceNormal);	
	return (AccelInSurface < 0.0f) ? 0.0f : AccelInSurface * ObjMass * FrictionCoeff;
}

FVector AKCCPhysics::CalculateFrictionAccel(const FVector Vel, const FVector& SurfaceNormal, const FVector& CurrentAccel, const float& ObjMass, const float& InvertedMass, const float& FrictionCoeff)
{
	// Realized issue with KCC is that more worse surface normal means that gravity is going against both gravity more as well as friction
	return -SafeNormalized(ProjectOnPlane(Vel, SurfaceNormal)) * CalculateNormalForce(SurfaceNormal, CurrentAccel, ObjMass, FrictionCoeff) * InvertedMass;
}

FVector AKCCPhysics::CalculateDragAccel(const FVector& Vel, const float& DragCoeff, const float& InvertedMass)
{
	return -Vel * DragCoeff * InvertedMass;
}

FVector AKCCPhysics::CalculateGravityAccel(const FVector& GravityDir, const float& GravityMag)
{
	return GravityDir * GravityMag;
}

inline void AKCCPhysics::CalculateMomentum(const FVector& ObjVelocity, const float& ObjMass, FVector& ObjMomentum)
{
	ObjMomentum = ObjVelocity * ObjMass;
}

void AKCCPhysics::CalculateBounceImpulse(const FVector& RelativeVelocity, const float& TotalMass, const FVector& SurfaceNormal , float& Impulse)
{
	Impulse = -(1 + CoefficientOfRestitution) * DotProduct(RelativeVelocity, SurfaceNormal);
	Impulse *= TotalMass;
}
#pragma endregion

inline float AKCCPhysics::Square(const float& NumberToSquare)
{
	return NumberToSquare * NumberToSquare;
}

inline float AKCCPhysics::Power(const float& MultNum, const int& Power)
{
	if (Power < 2)	return MultNum;	// Ensures that power isn't too low

	float PoweredNum = MultNum;	 // Stores for getting total number to the power of

	for (int i = 1; i < Power; ++i)	// Uses ++i as its minutely faster and skips over the current number
		PoweredNum *= MultNum;

	return PoweredNum;
}

FVector AKCCPhysics::ConvertToUE5Units(const FVector& Vector)
{
	return Vector * 100.0f;
}
FVector AKCCPhysics::ConvertFromUE5Units(const FVector& Vector)
{
	return Vector * 0.01f;	// Equivalent of 1/100 which is the equaivalent of /100 without the decreased performance
}
float AKCCPhysics::ConvertToUE5Units(const float& NumToConvert)
{
	return NumToConvert * 100.0f;
}
float AKCCPhysics::ConvertFromUE5Units(const float& NumToConvert)
{
	return NumToConvert * 0.01f;
}
#pragma region VectorMathematics

FVector AKCCPhysics::SafeNormalized(const FVector& FullVector)
{
	float SqrMag = DotProduct(FullVector, FullVector);
	if (SqrMag == 1.0f)	// Added Safety checks
		return FullVector;
	else if (FullVector.IsNearlyZero())
		return FVector::ZeroVector;

	return FullVector / FMath::Sqrt(SqrMag);
}

FVector AKCCPhysics::UnSafeNormalized(const FVector& FullVector)
{
	return FullVector / Magnitude(FullVector);
}

float AKCCPhysics::Magnitude(const FVector& FullVector)
{
	return FMath::Sqrt(DotProduct(FullVector, FullVector));
}

FVector AKCCPhysics::ProjectOnVector(const FVector& VectorToProject, const FVector& ProjectionVector)
{
	if (ProjectionVector.IsNearlyZero())
		return FVector::ZeroVector;

	return DotProduct(VectorToProject, ProjectionVector) / DotProduct(ProjectionVector, ProjectionVector) * ProjectionVector;
}

FVector AKCCPhysics::ProjectOnNormal(const FVector& VectorToProject, const FVector& ProjectionNormal)
{
	return DotProduct(VectorToProject, ProjectionNormal) * ProjectionNormal;
}

FVector AKCCPhysics::ProjectOnPlane(const FVector& FullVector, const FVector& PlaneNormal)
{
	return FullVector - ProjectOnVector(FullVector, PlaneNormal);
}

FVector AKCCPhysics::ProjectOnNormalizedPlane(const FVector& FullVector, const FVector& PlaneNormal)
{
	return FullVector - ProjectOnNormal(FullVector, PlaneNormal);
}

float AKCCPhysics::DotProduct(const FVector& FullVector, const FVector& VectorNormal)
{
	return FullVector.X * VectorNormal.X + FullVector.Y * VectorNormal.Y + FullVector.Z * VectorNormal.Z;
}

FVector AKCCPhysics::CrossProduct(const FVector& Vector1, const FVector& Vector2)
{
	return FVector(Vector1.Y * Vector2.Z - Vector1.Z * Vector2.Y, Vector1.Z * Vector2.X - Vector1.X * Vector2.Z, Vector1.X * Vector2.Y - Vector1.Y * Vector2.X);
}

FVector AKCCPhysics::RotateVector(const FVector& VectorToRotate, const FVector& Axis, float Angle, bool IsDegrees)
{
	Angle = (IsDegrees) ? Angle * (PI/180): Angle;
	return VectorToRotate * FMath::Cos(Angle) + CrossProduct(Axis, VectorToRotate) * FMath::Sin(Angle) + Axis * DotProduct(Axis, VectorToRotate) * (1.0f - FMath::Cos(Angle));
}

FVector AKCCPhysics::ReflectVectorOnNormal(const FVector& VectorToReflect, const FVector& ReflectionNormal)
{
	return VectorToReflect - 2 * DotProduct(VectorToReflect, ReflectionNormal) * ReflectionNormal;
}

float AKCCPhysics::AngleBetweenVectors(const FVector& Vector1, const FVector& Vector2)
{
	return FMath::Acos(DotProduct(Vector1, Vector2) / (Magnitude(Vector1) * Magnitude(Vector2))) * (180 / PI);
}

FVector AKCCPhysics::ProjectAndScaleNormalized(const FVector& FullVector, const FVector& PlaneNormal)
{
	// This is not the cause there is some loss of extremely small vectors but nothing much else and doesn't sync with the staggering of the KCC
	return SafeNormalized(ProjectOnNormalizedPlane(FullVector, PlaneNormal)) * Magnitude(FullVector);
}

FVector AKCCPhysics::ProjectAndScale(const FVector& FullVector, const FVector& PlaneNormal)
{
	// This is not the cause there is some loss of extremely small vectors but nothing much else and doesn't sync with the staggering of the KCC
	return SafeNormalized(ProjectOnPlane(FullVector, PlaneNormal)) * Magnitude(FullVector);
}
#pragma endregion
#pragma region TimeIntegrationMethods

void AKCCPhysics::CalculateEulerPosition(FVector& NewPosition, FVector& NewVelocity, const float& DeltaTime)
{
	NewVelocity += Acceleration * DeltaTime;
	NewPosition += NewVelocity * DeltaTime;
}

void AKCCPhysics::CalculateVelocityVerletPosition(FVector& NewDisplacement, FVector& NewVelocity, const float& DeltaTime)
{
	// Should technically use previous acceleration but might use acceleration to keep responsiveness
	NewDisplacement += NewVelocity * DeltaTime + 0.5f * PreviousAcceleration * Square(DeltaTime);
	NewVelocity += 0.5f * (PreviousAcceleration + Acceleration) * DeltaTime;	// Needs to be done afterwards as Acceleration is seperately accounted for via velocity verlet equation
}

void AKCCPhysics::CalculateVerletPosition(const FVector& PrevPos, FVector& NewPos, FVector& NewVelocity, const float& DeltaTime)
{
	FVector CurrentLocation = NewPos;
	NewPos = 2 * CurrentLocation - PrevPos + Acceleration * Square(DeltaTime);	// Multiplied by 2 to get location but am now getting displacement instead
	NewVelocity = (NewPos - CurrentLocation)/DeltaTime;	// Using Forward difference as setting future velocity and don't need to get a larger average
	// Symmetric Velocity Estimation would go over both timesteps as in previous -> current -> future and estimate based on that with double the deltatime
}

void AKCCPhysics::CalculateRK4Position(FVector& NewPosition, FVector& NewVelocity, const float& DeltaTime)
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
	
	NewPosition += (k1.PositionDerivative + 2 * k2.PositionDerivative + 2 * k3.PositionDerivative + k4.PositionDerivative) * (DeltaTime/6.0f);

	NewVelocity += (k1.VelocityDerivative + 2 * k2.VelocityDerivative + 2 * k3.VelocityDerivative + k4.VelocityDerivative) * (DeltaTime/6.0f);
}

void AKCCPhysics::CalculateRK4Acceleration(const FVector& Position, const FVector& CurrentVelocity, FVector& ComputedAccel)
{
	ComputedAccel = Acceleration;
}

#pragma endregion