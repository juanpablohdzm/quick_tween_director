// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "QTDStepData.h"
#include "Utils/LoopType.h"
#include "QuickTweenDirectorAsset.generated.h"

/**
 * Serializable asset that stores a complete tween-sequence definition.
 *
 * Create one from the Content Browser (right-click → QuickTween → Director Sequence).
 * Open it by double-clicking to launch the visual timeline editor.
 * At runtime, pass the asset to UQuickTweenDirectorPlayer to play the animation.
 */
UCLASS(BlueprintType)
class QUICKTWEENDIRECTOR_API UQuickTweenDirectorAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// ── Steps & tracks ────────────────────────────────────────────────────────

	/** All animation steps stored in this sequence. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director")
	TArray<FQTDStepData> Steps;

	/** Track definitions used by the editor to group steps visually. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director|Editor")
	TArray<FQTDTrackData> Tracks;

	// ── Global playback settings ──────────────────────────────────────────────

	/** How many times the full animation loops (-1 = infinite). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Playback")
	int32 Loops = 1;

	/** Loop behavior for the full animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Playback")
	ELoopType LoopType = ELoopType::Restart;

	/** Whether the player auto-kills itself when the animation finishes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Playback")
	bool bAutoKill = true;

	/** Whether the animation continues while the game is paused. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Playback")
	bool bPlayWhilePaused = false;

	// ── Accessors ─────────────────────────────────────────────────────────────

	/** Returns max(step.StartTime + step.Duration * step.Loops) across all steps. */
	UFUNCTION(BlueprintCallable, Category = "Director")
	float GetTotalDuration() const;

	// ── Editor helpers (called from the editor toolkit) ───────────────────────

	/** Creates a new track and returns its ID. */
	FGuid AddTrack(const FString& Label, EQTDStepType DefaultType = EQTDStepType::Vector);

	/** Removes a track and every step that belongs to it. Marks the asset dirty. */
	void RemoveTrack(const FGuid& InTrackId);

	/** Appends a step and returns its index. Marks the asset dirty. Generates a StepId if the step has none. */
	int32 AddStep(FQTDStepData InStep);

	/** Removes the step with the matching StepId. Marks the asset dirty. */
	void RemoveStep(const FGuid& InStepId);

	/** Updates an existing step (matched by StepId). Marks the asset dirty. */
	void UpdateStep(const FQTDStepData& InStep);

	/** Returns pointers to all steps belonging to the given track. */
	TArray<FQTDStepData*> GetStepsForTrack(const FGuid& InTrackId);

	/** Finds a step by its ID, returns nullptr if not found. */
	FQTDStepData* FindStep(const FGuid& InStepId);

	/** Returns a copy of the track data for the given ID, or an empty struct. */
	FQTDTrackData* FindTrack(const FGuid& InTrackId);
};
