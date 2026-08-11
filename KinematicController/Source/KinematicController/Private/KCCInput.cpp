// Fill out your copyright notice in the Description page of Project Settings.


#include "KCCInput.h"

#pragma region Player Input

AKCCInput::AKCCInput()
{
	Skeleton = CreateDefaultSubobject<USkeletalMeshComponent>("Character Mesh");
	Skeleton->SetupAttachment(RootComponent);
}

void AKCCInput::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (Skeleton)
	{
		Skeleton->SetSkeletalMesh(CharMesh);
	}
}

void AKCCInput::AsyncPhysicsTickActor(float DeltaTime, float SimTime)
{
	if (Camera && PlayerController)
	{
		AddMovementInput(Camera, DeltaTime);
		JumpTimerLogic(DeltaTime);
		JumpLogic();
	}
	Super::AsyncPhysicsTickActor(DeltaTime, SimTime);

	if (PlayerController)
	{
		InputManager->TempResetKey(EKeys::W);
		InputManager->TempResetKey(EKeys::A);
		InputManager->TempResetKey(EKeys::S);
		InputManager->TempResetKey(EKeys::D);
	}
}

void AKCCInput::CalculatePhysicsForces()
{
	AddForce(CalculateGravityAccel(GravityNormal, GravityMagnitude), ForceType::Acceleration);

	float VelMag = Magnitude(Velocity);

	if(IsGrounded)
		AddForce(CalculateDragAccel(Velocity, GroundDragCoefficient, InvMass), ForceType::Acceleration);
	else
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

void AKCCInput::PossessedBy(AController* NewController)
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
void AKCCInput::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Need to get local player to be able to get subsystem
	if (UEnhancedInputComponent* UserInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		UserInput->BindAction(MoveButton, ETriggerEvent::Triggered, this, &AKCCInput::Move);
		UserInput->BindAction(TurnCamAction, ETriggerEvent::Triggered, this, &AKCCInput::TurnCam);
		UserInput->BindAction(JumpButton, ETriggerEvent::Started, this, &AKCCInput::JumpPressed);
		UserInput->BindAction(JumpButton, ETriggerEvent::Triggered, this, &AKCCInput::JumpInput);
	}
}

void AKCCInput::Move(const FInputActionValue& InputVal)
{
	FVector2D AxisVal = InputVal.Get<FVector2D>();	// Stores 2D WASD value
	InputKey* CurrentKey;
	if (AxisVal.Y > 0 && (CurrentKey = InputManager->GetInputKey(EKeys::W)))	// Checks AxisVal Y and if getting the input key is valid then it updates the key in the input manager
		InputManager->UpdateKeyData(CurrentKey->Key);

	else if (AxisVal.Y < 0 && (CurrentKey = InputManager->GetInputKey(EKeys::S)))
		InputManager->UpdateKeyData(CurrentKey->Key);	// Uses the worlds real time delta seconds as I can't get it from InputVal

	// Checks AxisVal X and if getting the input key is valid then it updates the key in the input manager
	if (AxisVal.X < 0 && (CurrentKey = InputManager->GetInputKey(EKeys::A)))
		InputManager->UpdateKeyData(CurrentKey->Key);

	else if (AxisVal.X > 0 && (CurrentKey = InputManager->GetInputKey(EKeys::D)))
		InputManager->UpdateKeyData(CurrentKey->Key);
}

void AKCCInput::TurnCam(const FInputActionValue& InputVal)
{
	if (Camera)
	{
		Camera->CameraMovementAxis = InputVal.Get<FVector2D>();
	}
}

void AKCCInput::JumpInput(const FInputActionValue& InputVal)
{
	if (InputVal.Get<bool>())
	{
		InputManager->UpdateKeyData(EKeys::SpaceBar, GetWorld()->DeltaRealTimeSeconds);
	}
}

void AKCCInput::JumpPressed(const FInputActionValue& InputVal)
{
	if (InputVal.Get<bool>())
	{
		InputKey* Key = InputManager->GetInputKey(EKeys::SpaceBar);

		if (Key)
			Key->IsPressed = true;
	}
}


void AKCCInput::AddPlayerInputKeys()
{
	int MinimumInputFrames = 1;
	InputManager = Cast<UGI_InputManager>(GetGameInstance());
	InputManager->AddInputKey(EKeys::W, MinimumInputFrames);
	InputManager->AddInputKey(EKeys::S, MinimumInputFrames);
	InputManager->AddInputKey(EKeys::A, MinimumInputFrames);
	InputManager->AddInputKey(EKeys::D, MinimumInputFrames);
	InputManager->AddInputKey(EKeys::SpaceBar, MinimumInputFrames);
}

void AKCCInput::AddMovementInput(AActor* MovementAxis, const float& DeltaTime)
{
	//FVector MovementNormal = (IsGrounded) ? FloorNormal : GravityNormal;	// This is good for transform but worse for physics based as some movement is removed when seperated into plane and gravity normal
	FVector MovementNormal = GravityNormal;
	FVector VelocityXZ = ProjectOnNormalizedPlane(Velocity, MovementNormal);

	if (Magnitude(VelocityXZ) > MaxSpeed)
		return;

	FVector MovementForce = FVector::ZeroVector;
	FVector TransformForwardXZ = SafeNormalized(ProjectOnNormalizedPlane(MovementAxis->GetActorForwardVector(), MovementNormal));
	FVector TransformRightXZ = SafeNormalized(ProjectOnNormalizedPlane(MovementAxis->GetActorRightVector(), MovementNormal));

	InputKey* Key;

	if ((Key = InputManager->GetInputKey(EKeys::W)) && Key->IsDown)	// Tidied up so that GetInput Key isn't called twice
	{
		MovementForce += TransformForwardXZ;
	}
	if ((Key = InputManager->GetInputKey(EKeys::S)) && Key->IsDown)
	{
		MovementForce -= TransformForwardXZ;

	}
	if ((Key = InputManager->GetInputKey(EKeys::A)) && Key->IsDown)
	{
		MovementForce -= TransformRightXZ;

	}
	if ((Key = InputManager->GetInputKey(EKeys::D)) && Key->IsDown)
	{
		MovementForce += TransformRightXZ;
	}

	MovementForce = SafeNormalized(MovementForce);

	if (MovementForce.IsNearlyZero())
		return;

	RotateToMovement(SafeNormalized(ProjectOnNormalizedPlane(MovementForce, GravityNormal)), DeltaTime);	 // Should Rotate player towards movement force

	FVector DriftForce = -ProjectOnNormalizedPlane(VelocityXZ, MovementForce) * CorneringStiffness;

	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Current Move Mod:" + FString::SanitizeFloat(CalculateSpeedMod(VelocityXZ, MovementForce)));

	MovementForce = MovementForce * MoveSpeed * CalculateSpeedMod(VelocityXZ, MovementForce);	// At this operation Movement force becomes a nan(ind) num since its added to accel accel becomes this to hence confusing the whole system


	// Should go roughly
	//AddTransformVel(MovementForce);
	AddForce(MovementForce + DriftForce, ForceType::Acceleration);	// This for whatever reason is just disabling the physics no clue why
}
float AKCCInput::CalculateSpeedMod(const FVector& CurrentVelocity, const FVector& MovementDir)
{
	if (!SpeedCurve || !CorneringCurve)
		return 1.0f;

	float VelMag = Magnitude(CurrentVelocity);

	if (!IsGrounded)	// This whole function is what is likely causing most of my issues
	{
		return AirSpeed * CorneringCurve->FloatCurve.Eval(DotProduct(CurrentVelocity / VelMag, MovementDir));
	}
	return SpeedCurve->FloatCurve.Eval(VelMag / MaxSpeed) * CorneringCurve->FloatCurve.Eval(DotProduct(CurrentVelocity / VelMag, MovementDir));
}

void AKCCInput::JumpLogic()
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

	bool CanJump = Key->IsDown || (JumpBufferTimer > 0.0f && IsGrounded) || (!IsGrounded && CoyoteTimer > 0.0f && Key->IsDown);

	CanJump = CanJump && CurrentJumpCount < MaxJumpCount && (CurrentJumpCount < 1 || (JumpTimer > MinJumpTime && Key->IsPressed));	// Aded min Jump time check so that double jumps have a min jump time

	float UpwardVel = DotProduct(Velocity, GravityNormal);

	bool ShouldFall = JumpTimer > MinJumpTime && !Key->IsDown && !IsGrounded && !HasFallen && UpwardVel >= 0.0f;

	if (CanJump)
	{
		AddForce(JumpMagnitude * GravityNormal * Mass, ForceType::Impulse);
		++CurrentJumpCount;
		JumpTimer = 0.0f;
		Velocity -= ProjectOnNormal(Velocity, GravityNormal);
	}
	else if (ShouldFall)
	{
		AddForce(VariableHeightImp * -GravityNormal * Mass, ForceType::Impulse);
		HasFallen = true;
	}

	InputManager->TempResetKey(EKeys::SpaceBar);
	InputManager->OnKeyRelease(EKeys::SpaceBar);

}
void AKCCInput::JumpTimerLogic(const float& DeltaTime)
{
	if (IsGrounded)
	{
		CoyoteTimer = CoyoteTime;
	}
	else if (CoyoteTimer > 0.0f && !IsGrounded)
	{
		CoyoteTimer -= DeltaTime;
	}

	if (InputManager->GetInputKey(EKeys::SpaceBar)->IsDown && !IsGrounded)
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
void AKCCInput::RotateToMovement(const FVector& MovementVector, const float& DeltaTime)
{
	if (MovementVector.IsNearlyZero())
		return;
	FRotator MovementRotation = FQuat::Slerp(GetActorQuat(), SafeNormalized(MovementVector).Rotation().Quaternion(), DeltaTime * 2).Rotator();
	SetActorRotation(MovementRotation);
}
#pragma endregion