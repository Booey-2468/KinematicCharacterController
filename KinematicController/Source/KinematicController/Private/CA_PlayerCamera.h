// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "CA_PlayerCamera.generated.h"

/**
 * 
 */
UCLASS()
class ACA_PlayerCamera : public ACameraActor
{
	GENERATED_BODY()
	
	ACA_PlayerCamera();

	float CameraMaxDist = 1000.0f;
	float CameraMinDist = 50.0f;

	float MouseSensitivity = 300.0f;

	float LerpMaxDuration = 0.05f;	// Remove the Lerp functionality for the most smoothness but means can be more snappy to sudden changes
	float CurrentDeltaTime = 0.0f;	// The lerping can seem slightly jittery but gives a nice delayed effect
									// Added Lerping to Pitch and Yaw this defintitely helped with jitter though this whole system has the flaw that it can coincide with other collisions as its lerping
	void MoveCamera(const float& DeltaTime);
	
	virtual void BeginPlay() override;
public:
	virtual void Tick(float DeltaTime) override;
	AActor* FocusedActor = nullptr;
	FVector2D CameraMovementAxis = FVector2d::ZeroVector;





};
