// Fill out your copyright notice in the Description page of Project Settings.


#include "GI_InputManager.h"

UGI_InputManager::~UGI_InputManager()
{
	if (InputKeys.Num() < 1)
		return;
	for (int i = 0; i < InputKeys.Num(); ++i)	// Uses ++i as its slightly quicker
	{
		delete InputKeys[i];
	}
	InputKeys.Empty();
}

void UGI_InputManager::AddInputKey(const FKey& AddedKey, const int& MinFrames)
{
	if (InputKeys.Num() > 0)	// Only checked if InputKeys is not empty
	{
		for (int i = 0; i < InputKeys.Num(); ++i)	// Uses ++i as its slightly quicker
		{
			if (InputKeys[i]->Key == AddedKey)		// Now used to make sure multiple instances aren't added at once
				return;
		}
	}
	InputKey* OriginalKey = new InputKey();
	OriginalKey->Key = AddedKey;
	OriginalKey->MinFrames = MinFrames;
	InputKeys.Emplace(OriginalKey);
}

void UGI_InputManager::RemoveInputKey(const FKey& RemovedKey)
{
	if (InputKeys.Num() < 1)
		return;
	for (int i = 0; i < InputKeys.Num(); ++i)	// Uses ++i as its slightly quicker
	{
		if (InputKeys[i]->Key == RemovedKey)
		{
			delete InputKeys[i];
			InputKeys[i] = nullptr;
			InputKeys.RemoveAt(i);
			return;
		}
	}
}

void UGI_InputManager::TempResetKey(const FKey& KeyToReset)
{
	if (InputKeys.Num() < 1)
		return;
	for (int i = 0; i < InputKeys.Num(); ++i)	// Uses ++i as its slightly quicker
	{
		if (InputKeys[i]->Key == KeyToReset)
		{
			InputKeys[i]->FrameCount = 0;
			InputKeys[i]->HasBeenPressed = false;
			InputKeys[i]->MinFrames = 0;
			return;
		}
	}
}

InputKey* UGI_InputManager::GetInputKey(const FKey& KeyToGet)
{
	if (InputKeys.Num() < 1)
		return nullptr;
	for (int i = 0; i < InputKeys.Num(); ++i)	// Uses ++i as its slightly quicker
	{
		if (InputKeys[i]->Key == KeyToGet)
		{
			return InputKeys[i];
		}
	}
	return nullptr;
}

void UGI_InputManager::UpdateKeyData(const FKey& KeyToUpdate, float DeltaTime)
{
	InputKey* Key = GetInputKey(KeyToUpdate);

	if (Key)
	{
		++Key->FrameCount;
		if (Key->FrameCount > Key->MinFrames && !Key->HasBeenPressed)
		{
			Key->HasBeenPressed = true;
		}
		if(DeltaTime > 0)
			Key->HeldTime += DeltaTime;
	}
}

void UGI_InputManager::OnKeyRelease(const FKey& ReleasedKey)
{
	InputKey* Key = GetInputKey(ReleasedKey);
	if (Key)
	{
		Key->HeldTime = 0.0f;
	}
}
