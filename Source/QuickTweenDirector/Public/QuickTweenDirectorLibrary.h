// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "QuickTweenDirectorLibrary.generated.h"

class UQuickTweenDirectorAsset;
class UQuickTweenDirectorPlayer;

/**
 * Blueprint function library providing convenience functions for QuickTweenDirector.
 */
UCLASS()
class QUICKTWEENDIRECTOR_API UQuickTweenDirectorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Create a director player for the given asset and register it with the tween manager.
	 * The player is idle after creation — call BindSlot() for each named slot and then Play().
	 *
	 * @param WorldContextObject  Any in-world UObject.
	 * @param Asset               The director asset to instantiate.
	 * @return                    The new player, or nullptr on failure.
	 */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "QuickTweenDirector")
	static UQuickTweenDirectorPlayer* CreateDirectorPlayer(
		UObject* WorldContextObject,
		UQuickTweenDirectorAsset* Asset);

	/**
	 * Convenience: create the player, bind a single slot, and call Play() immediately.
	 * Useful for simple one-slot animations.
	 *
	 * @param WorldContextObject  Any in-world UObject.
	 * @param Asset               The director asset to play.
	 * @param SlotName            Name of the slot to bind (must match FQTDStepData::SlotName).
	 * @param SlotObject          The actor or component to animate.
	 * @return                    The playing player, or nullptr on failure.
	 */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "QuickTweenDirector")
	static UQuickTweenDirectorPlayer* PlayDirectorAsset(
		UObject* WorldContextObject,
		UQuickTweenDirectorAsset* Asset,
		FName SlotName,
		UObject* SlotObject);
};
