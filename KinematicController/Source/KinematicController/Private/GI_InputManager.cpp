// Fill out your copyright notice in the Description page of Project Settings.


#include "GI_InputManager.h"
/*
void UGI_InputManager::AddInputKey(const FKey& AddedKey)
{
	InputKey* OriginalKey = new InputKey();
	OriginalKey->Key = AddedKey.GetFName();
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

void UGI_InputManager::ResetKey(const FKey& KeyToReset)
{
	for (int i = 0; i <= InputKeys.Num(); ++i)	// Uses ++i as its slightly quicker
	{
		if (InputKeys[i]->Key == KeyToReset.GetFName())
		{
			InputKeys[i]->FrameCount = 0;
			InputKeys[i]->HasBeenPressed = false;
			InputKeys[i]->HeldTime = 0.0f;
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

void UGI_InputManager::UpdateKeyData(UPlayerController* InputReciever)
{
	for (int i = 0; i <= InputKeys.Num(); ++i)	// Uses ++i as its slightly quicker
	{
		if ( InputReciever-> (EKeys::GetKey(InputKeys[i])))
		{

		}
	}
}*/
