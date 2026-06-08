// Fill out your copyright notice in the Description page of Project Settings.


#include "GI_InputManager.h"

void UGI_InputManager::AddInputKey(const FKey& AddedKey, const int& MinFrames)
{
	InputKey* OriginalKey = new InputKey();
	OriginalKey->Key = AddedKey.GetFName();
	OriginalKey->MinFrames = MinFrames;
	InputKeys.Emplace(OriginalKey);
}

void UGI_InputManager::RemoveInputKey(const FKey& RemovedKey)
{
	for (int i = 0; i <= InputKeys.Num(); ++i)	// Uses ++i as its slightly quicker
	{
		if (InputKeys[i]->Key == RemovedKey.GetFName())
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
	for (int i = 0; i <= InputKeys.Num(); ++i)	// Uses ++i as its slightly quicker
	{
		if (InputKeys[i]->Key == KeyToReset.GetFName())
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
	for (int i = 0; i <= InputKeys.Num(); ++i)	// Uses ++i as its slightly quicker
	{
		if (InputKeys[i]->Key == KeyToGet.GetFName())
		{
			return InputKeys[i];
		}
	}
	return nullptr;
}

void UGI_InputManager::UpdateKeyData(const FKey& KeyToUpdate, const float& DeltaTime)
{
	InputKey* Key = GetInputKey(KeyToUpdate);

	if (Key)
	{
		++Key->FrameCount;
		if (Key->FrameCount > Key->MinFrames && !Key->HasBeenPressed)
		{
			Key->HasBeenPressed = true;
		}
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
