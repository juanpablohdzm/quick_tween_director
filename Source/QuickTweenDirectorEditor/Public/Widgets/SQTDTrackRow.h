// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "QTDStepData.h"

class UQuickTweenDirectorAsset;
class SQTDStepContent;
class SScrollBar;

DECLARE_DELEGATE_OneParam (FOnTrackDelete,  FGuid        /*TrackId*/);
/**
 * Fired when the user selects an animation type from the context submenu.
 * The step is pre-configured with StepType, SlotName (= ComponentVariableName),
 * VectorProperty / RotatorProperty, and StartTime.
 * The receiver opens SQTDStepDialog to collect From / To values.
 */
DECLARE_DELEGATE_OneParam (FOnStepAdded,    FQTDStepData /*PrefilledStep*/);
DECLARE_DELEGATE_OneParam (FOnStepEdit,     FQTDStepData /*Step*/);
DECLARE_DELEGATE_TwoParams(FOnStepMoved,    FGuid        /*StepId*/, float /*NewStartTime*/);
DECLARE_DELEGATE_OneParam (FOnStepDeleted,  FGuid        /*StepId*/);

/**
 * One row in the director timeline. Shows the track label (with component name and
 * type icon) on the left and all step boxes on the right.
 *
 * Step interactions:
 *   - Left-click + drag  → move (change StartTime)
 *   - Double-click       → open the step edit dialog
 *   - Right-click empty  → context submenu with animation types filtered by component class
 *   - Right-click step   → delete option
 */
class SQTDTrackRow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SQTDTrackRow) {}
		SLATE_ARGUMENT(FQTDTrackData,             Track)
		SLATE_ARGUMENT(UQuickTweenDirectorAsset*, Asset)
		SLATE_ARGUMENT(float,                     PixelsPerSec)
		SLATE_ARGUMENT(TSharedPtr<SScrollBar>,    HScrollBar)
		SLATE_EVENT (FOnTrackDelete,              OnTrackDelete)
		SLATE_EVENT (FOnStepAdded,                OnStepAdded)
		SLATE_EVENT (FOnStepEdit,                 OnStepEdit)
		SLATE_EVENT (FOnStepMoved,                OnStepMoved)
		SLATE_EVENT (FOnStepDeleted,              OnStepDeleted)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Update zoom and propagate to the inner content widget. */
	void SetPixelsPerSec(float NewPPS);

private:
	FQTDTrackData             Track;
	UQuickTweenDirectorAsset* Asset       = nullptr;
	float                     PixelsPerSec = 80.0f;

	FOnTrackDelete  OnTrackDelete;
	FOnStepAdded    OnStepAdded;
	FOnStepEdit     OnStepEdit;
	FOnStepMoved    OnStepMoved;
	FOnStepDeleted  OnStepDeleted;

	TSharedPtr<SQTDStepContent> StepContent;
};
