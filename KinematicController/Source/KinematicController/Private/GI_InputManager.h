// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "InputCoreTypes.h"

#include "GI_InputManager.generated.h"

struct InputKey
{
	float HeldTime = 0;
	int FrameCount = 0;
	int MinFrames = 0;
	bool HasBeenPressed = false;
	FKey* Key = nullptr;
};
UCLASS()
class UGI_InputManager : public UGameInstance
{
	GENERATED_BODY()

	TArray<InputKey*> InputKeys = TArray<InputKey*>();

public:
	void AddInputKey(FKey& AddedKey, const int& MinFrames);
	void RemoveInputKey(const FKey& RemovedKey);
	void TempResetKey(const FKey& KeyToReset);
	InputKey* GetInputKey(const FKey& KeyToGet);
	void UpdateKeyData(const FKey& KeyToUpdate, float DeltaTime = -1);
	void OnKeyRelease(const FKey& ReleasedKey);


	
};
