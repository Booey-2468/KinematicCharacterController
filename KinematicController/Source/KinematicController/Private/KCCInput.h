// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "KCCPhysics.h"

#include "GI_InputManager.h"

#include "CA_PlayerCamera.h"

#include "Components/TimelineComponent.h"

#include "EnhancedInput/Public/EnhancedInputSubsystems.h"

#include "Components/CapsuleComponent.h"

#include "EnhancedInputComponent.h"

#include "KCCInput.generated.h"

/**
 * 
 */
UCLASS()
class AKCCInput : public AKCCPhysics
{
	GENERATED_BODY()

	AKCCInput();
protected:
	UPROPERTY(EditAnywhere)
	USkeletalMesh* CharMesh;
	UPROPERTY(EditAnywhere)
	USkeletalMeshComponent* Skeleton;

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void AsyncPhysicsTickActor(float DeltaTime, float SimTime) override;

	virtual void CalculatePhysicsForces() override;

#pragma region Player Input

	UGI_InputManager* InputManager;

	UPROPERTY(EditAnywhere)
	UInputMappingContext* DefaultMappingContext;

	float MoveSpeed = 8.0f;	// Movement might be slightly slow but might be best to increase later

	float MaxSpeed = 6.0f;

	float AirSpeed = 0.3f;

	float GroundDragCoefficient = 5.0f; // Removes Velocity so that Velocity is instantly removed when on ground
	// Means that you can control the amoun of slide

	float CorneringStiffness = 3.0f;

	UPROPERTY(EditAnywhere)
	UCurveFloat* CorneringCurve;

	UPROPERTY(EditAnywhere)
	UCurveFloat* SpeedCurve;

	float JumpMagnitude = 7.0f;

	float VariableHeightImp = 2.0f;

	bool HasFallen = false;

	int MaxJumpCount = 2;
	int CurrentJumpCount = 0;

	float JumpBufferTime = 0.1f;

	float JumpBufferTimer = 0.0f;

	float CoyoteTime = 0.05f;

	float CoyoteTimer = 0.0f;

	float JumpTimer = 0.0f;

	float MinJumpTime = 0.2f;	// This essentially decides how high the minimum jump is can't use actual height as its not a good measure and can be effected by other things

	ACA_PlayerCamera* Camera = nullptr;

	APlayerController* PlayerController;

	UPROPERTY(EditAnywhere)
	UInputAction* MoveButton;
	UPROPERTY(EditAnywhere)
	UInputAction* TurnCamAction;
	UPROPERTY(EditAnywhere)
	UInputAction* JumpButton;

	virtual void PossessedBy(AController* NewController) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void Move(const FInputActionValue& InputVal);
	void TurnCam(const FInputActionValue& InputVal);
	void JumpInput(const FInputActionValue& InputVal);
	void JumpPressed(const FInputActionValue& InputVal);


	void AddPlayerInputKeys();

	void AddMovementInput(AActor* MovementAxis, const float& DeltaTime);

	float CalculateSpeedMod(const FVector& CurrentVelocity, const FVector& MovementDir);

	void JumpLogic();

	void JumpTimerLogic(const float& DeltaTime);

	void RotateToMovement(const FVector& MovementVector, const float& DeltaTime);
#pragma endregion
	
};
