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
	float CameraMinDist = 200.0f;

	float MouseSensitivity = 750.0f;

	float LerpMaxDuration = 0.2f;
	float CurrentDeltaTime = 0.0f;

	void MoveCamera(const float& DeltaTime);
	
	virtual void BeginPlay() override;
public:
	virtual void Tick(float DeltaTime) override;
	AActor* FocusedActor = nullptr;
	FVector2D CameraMovementAxis = FVector2d::ZeroVector;





};
