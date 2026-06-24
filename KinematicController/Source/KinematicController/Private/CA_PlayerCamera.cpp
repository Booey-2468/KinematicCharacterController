// Fill out your copyright notice in the Description page of Project Settings.


#include "CA_PlayerCamera.h"

ACA_PlayerCamera::ACA_PlayerCamera()
{
	PrimaryActorTick.bCanEverTick = true;
	
}

void ACA_PlayerCamera::MoveCamera(const float& DeltaTime)
{
	if (!FocusedActor)
		return;

	CurrentDeltaTime += DeltaTime;
	CurrentDeltaTime = FMath::Clamp(CurrentDeltaTime, 0.0f, LerpMaxDuration);

	if (CameraMovementAxis != FVector2D::ZeroVector)
	{
		FVector NewRotation = GetActorRotation().Euler();

		float PitchRot = (CameraMovementAxis.Y * MouseSensitivity * DeltaTime) + NewRotation.Y;
		float YawRot = (CameraMovementAxis.X * MouseSensitivity * DeltaTime) + NewRotation.Z;

		PitchRot = FMath::Lerp(NewRotation.Y, PitchRot, CurrentDeltaTime / LerpMaxDuration);
		YawRot = FMath::Lerp(NewRotation.Z, YawRot, CurrentDeltaTime / LerpMaxDuration);

		NewRotation = FVector(NewRotation.X, FMath::Clamp(PitchRot, -89.91f, 89.91f) , YawRot);	// Clamp cannot be above 89.91 to prevent gimbal locking as this prevents any yaw input working propely while in this state

		SetActorRotation(FRotator::MakeFromEuler(NewRotation));	// Don't use .Rotation for this as that doesn't handle euler only direction vectors
	}
	FVector CurrentLocation = FocusedActor->GetActorLocation();
	FVector CameraMovement = -GetActorForwardVector() * CameraMaxDist;
	
	FHitResult Hit;
	FCollisionShape CameraBounds = FCollisionShape::MakeSphere(10.0f);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(FocusedActor);

	GetWorld()->SweepSingleByChannel(Hit, CurrentLocation, CurrentLocation + CameraMovement, GetActorQuat(), ECollisionChannel::ECC_WorldStatic, CameraBounds, Params);

	if (Hit.bBlockingHit && Hit.Distance > CameraMinDist)
		CameraMovement = -GetActorForwardVector() * Hit.Distance;
	else if(Hit.bBlockingHit)
		CameraMovement = -GetActorForwardVector() * CameraMinDist;

	CurrentLocation = FMath::Lerp(GetActorLocation(), CurrentLocation + CameraMovement, CurrentDeltaTime/LerpMaxDuration);
	SetActorLocation(CurrentLocation);

	if(CameraMovementAxis != FVector2d::ZeroVector)
	{
		CameraMovementAxis = FVector2D::ZeroVector;
		CurrentDeltaTime = 0.0f;
	}
	
}
void ACA_PlayerCamera::BeginPlay()
{
	Super::BeginPlay();		// Needed for Tick to actually work assume it sets something up
}
void ACA_PlayerCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	MoveCamera(DeltaTime);
}
