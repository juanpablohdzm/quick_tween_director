// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "QTDStepData.h"

class UQuickTweenDirectorAsset;
class SQTDStepContent;

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
 * One track row in the director timeline.
 *
 * This widget shows ONLY the label column (component name, icon, delete button).
 * The step content widget is hosted separately (in the shared horizontal scroll box)
 * and can be retrieved via GetStepContentWidget().
 *
 * Step interactions (on the step content widget):
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
		SLATE_EVENT (FOnTrackDelete,              OnTrackDelete)
		SLATE_EVENT (FOnStepAdded,                OnStepAdded)
		SLATE_EVENT (FOnStepEdit,                 OnStepEdit)
		SLATE_EVENT (FOnStepMoved,                OnStepMoved)
		SLATE_EVENT (FOnStepDeleted,              OnStepDeleted)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Update zoom and propagate to the inner content widget. */
	void SetPixelsPerSec(float NewPPS);

	/**
	 * Returns the step content widget to be placed in the shared horizontal scroll box.
	 * Must be called after Construct().
	 */
	TSharedRef<SWidget> GetStepContentWidget() const;

private:
	FQTDTrackData             Track;
	UQuickTweenDirectorAsset* Asset        = nullptr;
	float                     PixelsPerSec = 80.0f;

	FOnTrackDelete  OnTrackDelete;
	FOnStepAdded    OnStepAdded;
	FOnStepEdit     OnStepEdit;
	FOnStepMoved    OnStepMoved;
	FOnStepDeleted  OnStepDeleted;

	TSharedPtr<SQTDStepContent> StepContent;
};
