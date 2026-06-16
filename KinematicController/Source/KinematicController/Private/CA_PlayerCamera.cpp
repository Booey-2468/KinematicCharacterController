// Fill out your copyright notice in the Description page of Project Settings.


#include "CA_PlayerCamera.h"

ACA_PlayerCamera::ACA_PlayerCamera()
{
	SetActorTickEnabled(true);
	PrimaryActorTick.bCanEverTick = true;
}

void ACA_PlayerCamera::MoveCamera(const float& DeltaTime)
{
	if (!FocusedActor)
		return;

	if (CameraMovementAxis != FVector2D::ZeroVector)
	{
		FVector NewRotation = GetActorRotation().Euler();

		float XAxisRot = CameraMovementAxis.X * MouseSensitivity * DeltaTime + NewRotation.X;
		float YAxisRot = CameraMovementAxis.Y * MouseSensitivity * DeltaTime + NewRotation.Y;

		NewRotation = FVector(FMath::Clamp(XAxisRot, -90.0f, 90.0f), YAxisRot, NewRotation.Z);

		SetActorRotation(NewRotation.Rotation());
	}
	FVector CurrentLocation = FocusedActor->GetActorLocation();
	FVector CameraMovement = -GetActorForwardVector() * CameraMaxDist;
	
	FHitResult Hit;
	FCollisionShape CameraBounds = FCollisionShape::MakeSphere(2.0f);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(FocusedActor);

	GetWorld()->SweepSingleByChannel(Hit, CurrentLocation, CurrentLocation + CameraMovement, GetActorQuat(), ECollisionChannel::ECC_WorldStatic, CameraBounds, Params);

	if (Hit.bBlockingHit && Hit.Distance > CameraMinDist)
		CameraMovement = -GetActorForwardVector() * Hit.Distance;
	else if(Hit.bBlockingHit)
		CameraMovement = -GetActorForwardVector() * CameraMinDist;

	CurrentLocation = FMath::Lerp(GetActorLocation(), CurrentLocation + CameraMovement, CurrentDeltaTime/LerpMaxDuration);
	SetActorLocation(CurrentLocation);

	if (CameraMovementAxis == FVector2D::ZeroVector)	// Increments by deltatime if camera is static allowing for linear camera movement and a minium amountof time for the camera to move positions
	{
		CurrentDeltaTime += DeltaTime;
		CurrentDeltaTime = FMath::Clamp(CurrentDeltaTime, 0.0f, LerpMaxDuration);
	}
	else
	{
		CameraMovementAxis = FVector2D::ZeroVector;
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
