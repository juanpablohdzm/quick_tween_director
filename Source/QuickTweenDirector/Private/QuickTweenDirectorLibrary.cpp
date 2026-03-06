// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#include "QuickTweenDirectorLibrary.h"
#include "QuickTweenDirectorAsset.h"
#include "QuickTweenDirectorPlayer.h"

UQuickTweenDirectorPlayer* UQuickTweenDirectorLibrary::CreateDirectorPlayer(
	UObject* WorldContextObject,
	UQuickTweenDirectorAsset* Asset)
{
	return UQuickTweenDirectorPlayer::Create(WorldContextObject, Asset);
}

UQuickTweenDirectorPlayer* UQuickTweenDirectorLibrary::PlayDirectorAsset(
	UObject* WorldContextObject,
	UQuickTweenDirectorAsset* Asset,
	FName SlotName,
	UObject* SlotObject)
{
	UQuickTweenDirectorPlayer* Player = UQuickTweenDirectorPlayer::Create(WorldContextObject, Asset);
	if (!Player) return nullptr;

	if (!SlotName.IsNone() && SlotObject)
	{
		Player->BindSlot(SlotName, SlotObject);
	}

	Player->Play();
	return Player;
}
