// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "InputCoreTypes.h"

#include "GI_InputManager.generated.h"

/**
 * 
 */
struct InputKey
{
	float HeldTime = 0;
	int FrameCount = 0;
	int MinFrames = 0;
	bool HasBeenPressed = false;
	string Key = "";
};

UCLASS()
class UGI_InputManager : public UGameInstance
{
	GENERATED_BODY()

	TArray<InputKey*> InputKeys;

	void AddInputKey(const FKey& AddedKey);
	void RemoveInputKey(const string& RemovedKey);
	void ResetKey(const string& KeyToReset);
	InputKey* GetInputKey(const string& KeyToGet);

	
};
